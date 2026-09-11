#ifndef RA_NATIVE_VIEW_H
#define RA_NATIVE_VIEW_H

inline int RA_ViewCeil(double value) { int n = (int)value; return n < value ? n+1 : n; }

// The original HUD owns [0,640). The battlefield owns a disjoint region of
// the indexed render atlas, so legacy gadgets cannot intercept world clicks.
enum { RA_WORLD_X = 640, RA_WORLD_Y = 16, RA_HUD_WIDTH = 160 };

struct RANativeView {
    int output_w, output_h;
    int world_w, world_h;
    int dest_x, dest_y, dest_w, dest_h;
    int zoom_index;
    double zoom;
};

inline double RA_ViewZoom(int index)
{
    static const double stops[] = {0.5, 2.0/3.0, 0.8, 1.0, 1.25, 1.5, 2.0};
    return stops[index < 0 ? 0 : (index > 6 ? 6 : index)];
}

inline RANativeView RA_MakeNativeView(int w, int h, int map_w, int map_h, int zoom)
{
    RANativeView v;
    v.output_w = w; v.output_h = h;
    v.zoom_index = zoom < 0 ? 0 : (zoom > 6 ? 6 : zoom);
    v.zoom = RA_ViewZoom(v.zoom_index);
    int available_w = w > RA_HUD_WIDTH ? w - RA_HUD_WIDTH : 1;
    int available_h = h > RA_WORLD_Y ? h - RA_WORLD_Y : 1;
    v.world_w = RA_ViewCeil(available_w / v.zoom);
    v.world_h = RA_ViewCeil(available_h / v.zoom);
    if (v.world_w > map_w) v.world_w = map_w;
    if (v.world_h > map_h) v.world_h = map_h;
    if (v.world_w < 1) v.world_w = 1;
    if (v.world_h < 1) v.world_h = 1;
    v.dest_w = RA_ViewCeil(v.world_w * v.zoom);
    v.dest_h = RA_ViewCeil(v.world_h * v.zoom);
    v.dest_x = v.dest_w < available_w ? (available_w - v.dest_w)/2 : 0;
    v.dest_y = RA_WORLD_Y + (v.dest_h < available_h ? (available_h - v.dest_h)/2 : 0);
    return v;
}

inline bool RA_ViewContainsWorld(RANativeView const &v, int x, int y)
{
    return x >= v.dest_x && y >= v.dest_y && x < v.dest_x + v.dest_w &&
        y < v.dest_y + v.dest_h && x < v.output_w - RA_HUD_WIDTH && y < v.output_h;
}

inline void RA_ViewPoint(RANativeView const &v, int x, int y, int *lx, int *ly)
{
    if (x < 0 || y < 0 || x >= v.output_w || y >= v.output_h) {
        *lx = -1; *ly = -1;
    } else if (RA_ViewContainsWorld(v, x, y)) {
        *lx = RA_WORLD_X + (int)((x - v.dest_x) / v.zoom);
        *ly = RA_WORLD_Y + (int)((y - v.dest_y) / v.zoom);
    } else if (x >= v.output_w - RA_HUD_WIDTH && y < 400) {
        *lx = 480 + x - (v.output_w - RA_HUD_WIDTH); *ly = y;
    } else if (y < RA_WORLD_Y && x < 160) {
        *lx = x; *ly = y;
    } else if (y < RA_WORLD_Y && x >= v.output_w - 320) {
        *lx = 320 + x - (v.output_w - 320); *ly = y;
    } else {
        *lx = -1; *ly = -1;
    }
}

#endif
