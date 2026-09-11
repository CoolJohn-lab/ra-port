#include "mac_sdl.h"
#include "mac_sdl_runtime.h"
#include <assert.h>
#include <string.h>
#include <vector>

LRESULT FAR PASCAL _export Windows_Procedure(HWND, UINT, WPARAM, LPARAM) { return 0; }

static void button(Uint32 type, Uint8 button, int x, int y)
{
    SDL_Event e; memset(&e, 0, sizeof(e));
    e.type=type; e.button.button=button; e.button.x=x; e.button.y=y;
    SDL_PushEvent(&e); MacSDL_PumpEvents();
}
int main()
{
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    assert(MacSDL_SetMode(640,400));
    RANativeView view=RA_MakeNativeView(640,400,2000,2000,3);
    MacSDL_NativeCommit(&view, true);
    SDL_Window *window=0;
    for (int id=1; id<100 && !window; ++id) {
        SDL_Window *candidate=SDL_GetWindowFromID(id);
        if (candidate && SDL_GetRenderer(candidate)) window=candidate;
    }
    assert(window);
    SDL_WarpMouseInWindow(window,200,200); MacSDL_PumpEvents();
    SDL_Event wheel; memset(&wheel,0,sizeof(wheel));
    wheel.type=SDL_MOUSEWHEEL; wheel.wheel.y=1;
#if SDL_VERSION_ATLEAST(2,0,18)
    wheel.wheel.preciseY=1.0f;
#endif
    SDL_PushEvent(&wheel); MacSDL_PumpEvents();
    RANativeView zoomed; double zx,zy; int anchorx,anchory;
    assert(MacSDL_NativeRequest(2000,2000,&zoomed,&zx,&zy,&anchorx,&anchory));
    assert(zoomed.zoom==1.25 && anchorx==200 && anchory==200);
    SDL_WarpMouseInWindow(window,520,200); MacSDL_PumpEvents();
    SDL_PushEvent(&wheel); MacSDL_PumpEvents();
    assert(MacSDL_NativeRequest(2000,2000,&zoomed,&zx,&zy,&anchorx,&anchory));
    assert(zoomed.zoom==1.25 && anchorx==-1); // sidebar owns wheel input
    button(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT,180,5);
    button(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT,180,5);
    assert(MacSDL_NativeRequest(2000,2000,&zoomed,&zx,&zy,&anchorx,&anchory));
    assert(zoomed.zoom==1.0);
    button(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_MIDDLE, 200, 200);
    assert(MacSDL_NativeDragging());
    SDL_Event e; memset(&e, 0, sizeof(e));
    e.type=SDL_MOUSEMOTION;e.motion.x=225;e.motion.y=190;e.motion.xrel=25;e.motion.yrel=-10;
    SDL_PushEvent(&e);MacSDL_PumpEvents();
    button(SDL_MOUSEBUTTONUP,SDL_BUTTON_MIDDLE,225,190);
    assert(!MacSDL_NativeDragging());
    RANativeView next;double dx,dy;int ax,ay;
    assert(MacSDL_NativeRequest(2000,2000,&next,&dx,&dy,&ax,&ay));
    assert(dx==-25 && dy==10);
    assert(MacSDL_NativeRequest(2000,2000,&next,&dx,&dy,&ax,&ay));
    assert(dx==0 && dy==0); // one-shot consumption
    button(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_MIDDLE,520,200);
    assert(!MacSDL_NativeDragging());
    button(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_MIDDLE,200,200);
    memset(&e,0,sizeof(e));e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    SDL_PushEvent(&e);MacSDL_PumpEvents();assert(!MacSDL_NativeDragging());
    MacSDL_NativeCommit(&view,false);
    button(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_MIDDLE,200,200);assert(!MacSDL_NativeDragging());
    button(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT,270,5);assert(MacSDL_NativeEffects());
    button(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT,270,5);
    // Exercise actual palette conversion/composition and visibility-masked light.
    std::vector<unsigned char> atlas((640+480)*400,100);
    unsigned char mapped[128*128]; memset(mapped,0,sizeof(mapped));
    RANativeLight light={200,184,40,128,255,128,0};
    MacSDL_NativeLights(&light,1,mapped,0,0);
    assert(MacSDL_NativePresent(&atlas[0],1120,400,1120));
    SDL_Renderer *renderer=0;
    for (int id=1; id<100 && !renderer; ++id) {
        SDL_Window *candidate=SDL_GetWindowFromID(id);
        if (candidate) renderer=SDL_GetRenderer(candidate);
    }
    assert(renderer);
    Uint32 pixel=0; SDL_Rect sample={200,200,1,1};
    assert(SDL_RenderReadPixels(renderer,&sample,SDL_PIXELFORMAT_ARGB8888,&pixel,4)==0);
    assert((pixel&0xffffff)==0x646464); // unmapped receiver stays exactly original
    memset(mapped,1,sizeof(mapped));
    MacSDL_NativeLights(&light,1,mapped,0,0);
    assert(MacSDL_NativePresent(&atlas[0],1120,400,1120));
    assert(SDL_RenderReadPixels(renderer,&sample,SDL_PIXELFORMAT_ARGB8888,&pixel,4)==0);
    assert(((pixel>>16)&255)>100 && (pixel&255)==100);
    atlas[(200)*1120+640+200]=0;
    assert(MacSDL_NativePresent(&atlas[0],1120,400,1120));
    assert(SDL_RenderReadPixels(renderer,&sample,SDL_PIXELFORMAT_ARGB8888,&pixel,4)==0);
    assert((pixel&0xffffff)==0); // palette black/shroud remains black
    MacSDL_NativeCommit(0,false);
    MacSDL_Shutdown();
    return 0;
}
