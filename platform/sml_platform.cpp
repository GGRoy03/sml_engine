// ===========================================
// Type Definitions
// ==========================================

constexpr sml_u32 MaxNameLength = 256;
constexpr sml_u32 MaxPathLength = 512;

struct sml_window
{
    void   *Handle;
    sml_i32 Width;
    sml_i32 Height;
};

enum { SML_KEYBOARD_KEY_COUNT = 256 };

struct sml_game_button_state
{
    bool    EndedDown;
    sml_u32 HalfTransitionCount;
};

struct sml_game_controller_input
{
    sml_game_button_state Buttons[SML_KEYBOARD_KEY_COUNT];

    sml_i32               MouseX, MouseY, MouseZ;
    sml_game_button_state MouseButtons[5];
};

struct platform_file
{
    char                   Name[MaxNameLength];
    char                   FullPath[MaxPathLength];
    bool                   IsDir;
    sml_u32                Parent;
    dynamic_array<sml_u32> Children;
};

// ===========================================
// User API
// ==========================================

sml_window Sml_CreateWindow(sml_i32 Width, sml_i32 Height, const char *Title);

bool Sml_UpdateWindow(sml_window *Window, sml_game_controller_input *Inputs);

bool Sml_IsKeyDown(char Char);

static dynamic_array<platform_file>
Platform_BuildFileTree(const char *RootUTF8);

// ===========================================
// Implementations
// ==========================================

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "third_party/imgui/backends/imgui_impl_win32.h"
#include "third_party/imgui/backends/imgui_impl_dx11.h"

#include "sml_platform_win32.cpp"

#endif
