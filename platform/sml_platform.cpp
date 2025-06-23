struct sml_window
{
    void   *Handle;
    sml_i32 Width;
    sml_i32 Height;
};

sml_window Sml_CreateWindow   (sml_i32 Width, sml_i32 Height, const char *Title);
bool       Sml_UpdateWindow   (sml_window *Window);

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sml_platform_win32.cpp"

#endif
