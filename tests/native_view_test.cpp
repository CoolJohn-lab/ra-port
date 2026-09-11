#include <assert.h>
#include "native_view.h"

int main()
{
    RANativeView small = RA_MakeNativeView(640, 400, 3024, 3024, 3);
    RANativeView large = RA_MakeNativeView(1920, 1080, 3024, 3024, 3);
    assert(small.world_w == 480 && small.world_h == 384);
    assert(large.world_w == 1760 && large.world_h == 1064);
    assert(small.zoom == large.zoom);
    for (int z = 0; z < 7; ++z) {
        RANativeView v = RA_MakeNativeView(1279, 799, 3024, 3024, z);
        int x, y;
        RA_ViewPoint(v, 111, 200, &x, &y);
        assert(x >= RA_WORLD_X && y >= RA_WORLD_Y);
        // Inverse transform differs by at most one source pixel (nearest sampling).
        double px = v.dest_x + (x-RA_WORLD_X)*v.zoom;
        assert(px <= 111 && 111-px < v.zoom+0.001);
        RA_ViewPoint(v, 1250, 200, &x, &y);
        assert(x == 611 && y == 200); // stable legacy build-button coordinates
        RA_ViewPoint(v, 1250, 650, &x, &y);
        assert(x == -1 && y == -1); // empty sidebar cannot issue orders
        RA_ViewPoint(v, 10, 5, &x, &y);
        assert(x == 10 && y == 5);
        assert(v.world_w <= 3024 && v.world_h <= 3024);
    }
    RANativeView tiny = RA_MakeNativeView(1920, 1080, 480, 384, 3);
    assert(tiny.dest_x == 640 && tiny.dest_y == 356);
    assert(!RA_ViewContainsWorld(tiny, 0, 200));
    assert(RA_ViewContainsWorld(tiny, 640, 356));
    assert(!RA_ViewContainsWorld(tiny, 1120, 356));
    assert(RA_MakeNativeView(640, 400, 1000, 1000, -90).zoom_index == 0);
    assert(RA_MakeNativeView(640, 400, 1000, 1000, 90).zoom_index == 6);
    return 0;
}
