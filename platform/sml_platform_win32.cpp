// ===========================================
// Internal Functions.
// ==========================================

static LRESULT CALLBACK
SmlWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch(Message)
    {

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProcW(Handle, Message, WParam, LParam);

    }
}

static void
SmlWin32_ProcessKeyboardMessage(sml_game_button_state *NewState, bool IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

static bool
SmlWin32_ProcessMessages(sml_game_controller_input *Keyboard)
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            u32  VKCode  = (u32)Message.wParam;
            bool WasDown = ((Message.lParam & (1 << 30)) != 0);
            bool IsDown  = ((Message.lParam & (1 << 31)) == 0);

            if(WasDown != IsDown && VKCode < SML_KEYBOARD_KEY_COUNT)
            {
                SmlWin32_ProcessKeyboardMessage(&Keyboard->Buttons[VKCode], IsDown);
            }

        } break;

        case WM_QUIT:
        {
            return false;
        } break;

        default:
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        } break;

        }
    }

    return true;
}

static HMODULE 
SmlWin32_GetModuleHandle()
{
    HMODULE ImageBase;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&SmlWin32_GetModuleHandle, &ImageBase)) 
    {
        return ImageBase;
    }

    return 0;
}

static void
SmlWin32_GetClientSize(HWND Handle, sml_i32 *OutWidth, sml_i32 *OutHeight)
{
    RECT Rect;
    GetClientRect(Handle, &Rect);
    *OutWidth  = Rect.right - Rect.left;
    *OutHeight = Rect.bottom - Rect.top;
}

// ===========================================
// User API implementation
// ==========================================

sml_window 
Sml_CreateWindow(sml_i32 Width, sml_i32 Height, const char *Title)
{
    Sml_Unused(Title);

    sml_window SmlWindow = {};

    WNDCLASSEXW WindowClass   = {};
    WindowClass.cbSize        = sizeof(WindowClass);
    WindowClass.lpfnWndProc   = SmlWin32_WindowProc;
    WindowClass.hInstance     = (HINSTANCE)SmlWin32_GetModuleHandle();
    WindowClass.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    WindowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    WindowClass.lpszClassName = L"type_game_window_name";

    ATOM Atom = RegisterClassExW(&WindowClass);
    Sml_Assert(Atom);

    DWORD EXStyle = WS_EX_APPWINDOW;
    DWORD Style   = WS_OVERLAPPEDWINDOW;

    SmlWindow.Handle = CreateWindowExW(
        EXStyle, WindowClass.lpszClassName, L"Window", Style,
        CW_USEDEFAULT, CW_USEDEFAULT, Width, Height,
        NULL, NULL, WindowClass.hInstance, NULL);

    HDC DeviceContext = GetDC((HWND)SmlWindow.Handle);
    Sml_Assert(DeviceContext);

    ShowWindow((HWND)SmlWindow.Handle, SW_SHOWDEFAULT);

    SmlWin32_GetClientSize((HWND)SmlWindow.Handle, &SmlWindow.Width, &SmlWindow.Height);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(SmlWindow.Handle);

    ImGui::StyleColorsDark();

    return SmlWindow;
}

bool 
Sml_UpdateWindow(sml_window *Window, sml_game_controller_input *Inputs)
{
    bool WindowClosed = SmlWin32_ProcessMessages(Inputs);
    if(!WindowClosed) return false;

    SmlWin32_GetClientSize((HWND)Window->Handle, &Window->Width, &Window->Height);

    ImGui_ImplWin32_NewFrame();

    return true;
}

bool Sml_IsKeyDown(char Char, sml_game_controller_input* Inputs)
{
    SHORT KeyScan = VkKeyScanA(Char);
    if(KeyScan == -1) return false;

    BYTE VKCode = LOBYTE(KeyScan);

    return Inputs->Buttons[VKCode].EndedDown;
}
