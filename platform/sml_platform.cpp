// ===========================================
// Internal Functions.
// ==========================================

struct sml_window
{
    void   *Handle;
    sml_i32 Width;
    sml_i32 Height;
};

enum { SML_KEYBOARD_KEY_COUNT = 256 };

typedef struct
{
    bool    EndedDown;
    sml_u32 HalfTransitionCount;
} sml_game_button_state;

typedef struct
{
    sml_game_button_state Buttons[SML_KEYBOARD_KEY_COUNT];

    sml_i32               MouseX, MouseY, MouseZ;
    sml_game_button_state MouseButtons[5];
} sml_game_controller_input;

// ===========================================
// Platform Agnostic helpers
// ==========================================



// ===========================================
// User API
// ==========================================

sml_window Sml_CreateWindow(sml_i32 Width, sml_i32 Height, const char *Title);

bool Sml_UpdateWindow(sml_window *Window, sml_game_controller_input *Inputs);

bool Sml_IsKeyDown(char Char);

// ===========================================
// Implementations
// ==========================================

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sml_platform_win32.cpp"

#endif
