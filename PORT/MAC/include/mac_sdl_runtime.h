#pragma once

#include <windows.h>

bool MacSDL_SetMode(int width, int height);
bool MacSDL_SetFullscreen(bool enabled);
bool MacSDL_GetFullscreen(void);
void MacSDL_Shutdown(void);
void MacSDL_SetPalette(PALETTEENTRY const *entries, int count);
void MacSDL_Present8(unsigned char const *pixels, int width, int height, int pitch);
void MacSDL_PumpEvents(void);
bool MacSDL_QuitRequested(void);
bool MacSDL_TouchCursorHidden(void);
int MacSDL_ConsumeMobilePointerDrag(int *x, int *y);

#include "native_view.h"
bool MacSDL_NativeRequest(int map_w, int map_h, RANativeView *view,
    double *pan_x, double *pan_y, int *anchor_x, int *anchor_y);
void MacSDL_NativeCommit(RANativeView const *view, bool controls);
bool MacSDL_NativeDragging(void);
void MacSDL_NativePointer(int *x, int *y);
void MacSDL_NativeCursor(unsigned char const *pixels, int w, int h, int hot_x, int hot_y);
bool MacSDL_NativePresent(unsigned char const *pixels, int width, int height, int pitch);
bool MacSDL_NativeEffects(void);
void MacSDL_NativeHelp(int x, int y, int w, int h);

struct RANativeLight { int x, y, radius, strength, red, green, blue; };
void MacSDL_NativeLights(RANativeLight const *lights, int count,
    unsigned char const *mapped_cells, int camera_x, int camera_y);
