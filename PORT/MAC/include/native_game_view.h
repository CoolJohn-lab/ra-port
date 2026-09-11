#ifndef RA_NATIVE_GAME_VIEW_H
#define RA_NATIVE_GAME_VIEW_H

void NativeView_Session(bool enabled);
void NativeView_Update(void);
void NativeView_Suspend(void);
bool NativeView_Active(void);
bool NativeView_Present(void);
bool NativeView_Dimensions(int &x, int &y, int &width_pixels, int &height_pixels);
void NativeView_Tesla(unsigned long coord);
void NativeView_ScrollPoint(int &x, int &y);
bool NativeView_ScreenEdge(void);

class NativeViewMovieScope {
public:
    NativeViewMovieScope();
    ~NativeViewMovieScope();
};

#endif
