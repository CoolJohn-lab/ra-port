// Linux presentation only. No state here is serialized or used by the simulation.
#include "../../../CODE/FUNCTION.H"
#include "native_game_view.h"
#include "mac_sdl_runtime.h"
#include <stdlib.h>


static bool SessionEnabled = false;
static bool Active = false;
static int MovieDepth = 0;
static RANativeView View;
static GraphicBufferClass *Atlas = 0;
static unsigned char *AtlasPixels = 0;
static double PanRemainderX = 0, PanRemainderY = 0;
struct TeslaFlash { COORDINATE coord; unsigned long tick; };
static TeslaFlash TeslaFlashes[16];
static int NextTeslaFlash = 0;

void NativeView_Tesla(unsigned long coord)
{
    if (!Active || !MacSDL_NativeEffects()) return;
    TeslaFlashes[NextTeslaFlash].coord = (COORDINATE)coord;
    TeslaFlashes[NextTeslaFlash].tick = TickCount;
    NextTeslaFlash = (NextTeslaFlash+1)%16;
}

static void collect_lights(void)
{
    RANativeLight lights[32];
    int count = 0;
    unsigned char mapped[128*128];
    memset(mapped, 0, sizeof(mapped));
    if (MacSDL_NativeEffects()) {
        for (int y=Map.MapCellY; y<Map.MapCellY+Map.MapCellHeight; ++y)
            for (int x=Map.MapCellX; x<Map.MapCellX+Map.MapCellWidth; ++x)
                mapped[y*128+x] = Map[XY_Cell(x,y)].IsMapped ? 1 : 0;
        for (int i=0; i<Anims.Count() && count<24; ++i) {
            AnimClass const *anim=Anims.Ptr(i);
            if (!anim || anim->IsInvisible || anim->Delay) continue;
            AnimType type = *anim;
            if (type < ANIM_FBALL1 || type > ANIM_NAPALM3) continue;
            COORDINATE coord=anim->Center_Coord();
            if (!Map[Coord_Cell(coord)].IsMapped) continue;
            int strength = 180 - anim->Fetch_Stage()*18;
            if (strength <= 0) continue;
            int x,y; if (!Map.Coord_To_Pixel(coord,x,y)) continue;
            RANativeLight light={x,y, type==ANIM_FBALL1 ? 70 : 42, strength,255,145,50};
            lights[count++]=light;
        }
        for (int i=0; i<16 && count<32; ++i) {
            TeslaFlash const &flash=TeslaFlashes[i];
            unsigned long age=TickCount-flash.tick;
            if (!flash.coord || age>=12 || !Map[Coord_Cell(flash.coord)].IsMapped) continue;
            int x,y; if (!Map.Coord_To_Pixel(flash.coord,x,y)) continue;
            RANativeLight light={x,y,64,(int)(180*(12-age)/12),100,165,255};
            lights[count++]=light;
        }
    }
    // Coordinates are absolute map pixels, not the virtual atlas offset.
    MacSDL_NativeLights(lights,count,mapped,
        (int)Coord_X(Map.TacticalCoord)*CELL_PIXEL_W/CELL_LEPTON_W,
        (int)Coord_Y(Map.TacticalCoord)*CELL_PIXEL_H/CELL_LEPTON_H);
}

bool NativeView_Active(void) { return Active; }

void NativeView_Suspend(void)
{
    if (!Active) return;
    Active = false;
    MacSDL_NativeCommit(0, false);
    HidPage.Attach(&HiddenPage, 0, ScreenHeight == 480 ? 40 : 0, 640, 400);
    WindowList[WINDOW_MAIN][WINDOWWIDTH] = 640;
    WindowList[WINDOW_MAIN][WINDOWHEIGHT] = 400;
    Map.Set_View_Dimensions(0, 16, 20, 16);
    Map.Help_Text(TXT_NONE);
    Map.Flag_To_Redraw(true);
    PanRemainderX = PanRemainderY = 0;
}

void NativeView_Session(bool enabled)
{
    char const *classic = getenv("RA_CLASSIC_VIEW");
    SessionEnabled = enabled && !(classic && atoi(classic));
    if (!enabled) {
        NativeView_Suspend();
        memset(TeslaFlashes, 0, sizeof(TeslaFlashes));
        delete Atlas;
        Atlas = 0;
        free(AtlasPixels);
        AtlasPixels = 0;
    }
}

NativeViewMovieScope::NativeViewMovieScope() { ++MovieDepth; NativeView_Suspend(); }
NativeViewMovieScope::~NativeViewMovieScope() { --MovieDepth; }

bool NativeView_Dimensions(int &x, int &y, int &w, int &h)
{
    if (!Active) return false;
    x = RA_WORLD_X; y = RA_WORLD_Y;
    w = View.world_w; h = View.world_h;
    return true;
}

void NativeView_Update(void)
{
    if (!SessionEnabled || !GameActive || MovieDepth || SpecialDialog != SDLG_NONE || Debug_Map) {
        NativeView_Suspend();
        return;
    }
    MacSDL_PumpEvents();
    RANativeView next;
    double pan_x, pan_y;
    int anchor_x, anchor_y;
    if (!MacSDL_NativeRequest(Map.MapCellWidth * CELL_PIXEL_W, Map.MapCellHeight * CELL_PIXEL_H,
        &next, &pan_x, &pan_y, &anchor_x, &anchor_y)) return;
    int w = RA_WORLD_X + next.world_w;
    int h = max(400, RA_WORLD_Y + next.world_h);
    bool changed = !Active || w != Atlas->Get_Width() || h != Atlas->Get_Height() ||
        next.world_w != View.world_w || next.world_h != View.world_h ||
        next.output_w != View.output_w || next.output_h != View.output_h || next.zoom_index != View.zoom_index ||
        Map.TacPixelX != RA_WORLD_X || Lepton_To_Pixel(Map.TacLeptonWidth) != next.world_w ||
        Lepton_To_Pixel(Map.TacLeptonHeight) != next.world_h;
    COORDINATE position = Map.DesiredTacticalCoord;
    if (Active && next.zoom_index != View.zoom_index) {
        if (anchor_x < 0) {
            anchor_x = (View.output_w - 160)/2;
            anchor_y = (View.output_h + 16)/2;
        }
        pan_x += (anchor_x-View.dest_x)/View.zoom - (anchor_x-next.dest_x)/next.zoom;
        pan_y += (anchor_y-View.dest_y)/View.zoom - (anchor_y-next.dest_y)/next.zoom;
    }
    if (!Atlas || w != Atlas->Get_Width() || h != Atlas->Get_Height()) {
        unsigned char *pixels = (unsigned char *)calloc((size_t)w, h);
        if (!pixels) {
            fprintf(stderr, "Native view allocation failed (%dx%d); retaining previous view\n", w, h);
            return;
        }
        GraphicBufferClass *replacement = new GraphicBufferClass(w, h, pixels, (long)w*h);
        if (!replacement) { free(pixels); return; }
        GraphicBufferClass *old = Atlas;
        unsigned char *old_pixels = AtlasPixels;
        Atlas = replacement; AtlasPixels = pixels;
        HidPage.Attach(Atlas, 0, 0, w, h);
        delete old;
        free(old_pixels);
    } else if (!Active) {
        HidPage.Attach(Atlas, 0, 0, w, h);
    }
    View = next;
    Active = true;
    WindowList[WINDOW_MAIN][WINDOWWIDTH] = w;
    WindowList[WINDOW_MAIN][WINDOWHEIGHT] = h;
    bool controls = !Map.IsRubberBand && !Map.PendingObject &&
        !Keyboard->Down(KN_LMOUSE) && !Keyboard->Down(KN_RMOUSE);
    MacSDL_NativeCommit(&View, controls);
    if (changed) {
        Map.Set_View_Dimensions(RA_WORLD_X, RA_WORLD_Y);
        Map.Help_Text(TXT_NONE);
        Map.Flag_To_Redraw(true);
    }
    if (pan_x || pan_y || changed) {
        // Signed conversion avoids the legacy unsigned LEPTON wrapping negative motion.
        double dx = pan_x * CELL_LEPTON_W / CELL_PIXEL_W + PanRemainderX;
        double dy = pan_y * CELL_LEPTON_H / CELL_PIXEL_H + PanRemainderY;
        int ix = (int)dx, iy = (int)dy;
        PanRemainderX = dx - ix; PanRemainderY = dy - iy;
        int x = (int)Coord_X(position) + ix, y = (int)Coord_Y(position) + iy;
        int minx = Map.MapCellX * CELL_LEPTON_W, miny = Map.MapCellY * CELL_LEPTON_H;
        int maxx = minx + Map.MapCellWidth * CELL_LEPTON_W - Map.TacLeptonWidth;
        int maxy = miny + Map.MapCellHeight * CELL_LEPTON_H - Map.TacLeptonHeight;
        x = max(minx, min(maxx, x)); y = max(miny, min(maxy, y));
        Map.Set_Tactical_Position(XY_Coord(x, y));
    }
    // Presentation controls and cursor movement require a present even on idle maps.
    Map.Flag_To_Redraw(false);
}

bool NativeView_Present(void)
{
    if (!Active || !AtlasPixels) return false;
    char label[40];
    sprintf(label, "%d%%", (int)(View.zoom*100+0.5));
    HidPage.Fill_Rect(160, 0, 319, 15, BLACK);
    Fancy_Text_Print("%s", 164, 3, &MetalScheme, TBLACK, TPF_6PT_GRAD|TPF_NOSHADOW, label);
    Fancy_Text_Print(MacSDL_NativeEffects() ? "LIGHT: ON" : "LIGHT: OFF", 244, 3,
        &MetalScheme, TBLACK, TPF_6PT_GRAD|TPF_NOSHADOW);
    collect_lights();
    MacSDL_NativePresent(AtlasPixels, Atlas->Get_Width(), Atlas->Get_Height(), Atlas->Get_Width());
    return true;
}

void NativeView_ScrollPoint(int &x, int &y)
{
    if (!Active) return;
    int px, py; MacSDL_NativePointer(&px, &py);
    x = px * 640 / View.output_w;
    y = py * 400 / View.output_h;
    if (px == View.output_w-1) x = 639;
    if (py == View.output_h-1) y = 399;
}

bool NativeView_ScreenEdge(void)
{
    int x, y; MacSDL_NativePointer(&x, &y);
    return x <= 0 || y <= 0 || x >= View.output_w-1 || y >= View.output_h-1;
}
