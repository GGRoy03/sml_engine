// ===========================================
// Internal Functions.
// ==========================================

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK
SmlWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if (ImGui_ImplWin32_WndProcHandler(Handle, Message, WParam, LParam))
    {
        return 0;
    }

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

            TranslateMessage(&Message);
            DispatchMessage(&Message);

        } break;

        case WM_QUIT:
        {
            return false;
        } break;

        default:
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
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

static void
SmlWin32_UTF8ToWide(const char *Str, wchar_t *OutBuffer, sml_i32 OutBufferSize)
{
    auto StrSize   = sml_i32(strlen(Str));
    auto SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, nullptr, 0);
    if(SizeNeeded > OutBufferSize) return;

    MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, OutBuffer, SizeNeeded);
}

static void
SmlWin32_WideToUTF8(const wchar_t *Str, char *OutBuffer, sml_i32 OutBufferSize)
{
    auto StrSz      = sml_i32(wcslen(Str));
    auto SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, Str, StrSz, NULL, 0, NULL, NULL);
    if(SizeNeeded > OutBufferSize) return;

    WideCharToMultiByte(CP_UTF8, 0, Str, StrSz, OutBuffer, SizeNeeded, NULL, NULL);
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
    bool WindowIsValid = SmlWin32_ProcessMessages(Inputs);
    if(!WindowIsValid) return false;

    SmlWin32_GetClientSize((HWND)Window->Handle, &Window->Width, &Window->Height);

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool Sml_IsKeyDown(char Char, sml_game_controller_input* Inputs)
{
    SHORT KeyScan = VkKeyScanA(Char);
    if(KeyScan == -1) return false;

    BYTE VKCode = LOBYTE(KeyScan);

    return Inputs->Buttons[VKCode].EndedDown;
}

// WARN:
// 1) Does not parse the root name

static dynamic_array<platform_file>
Platform_BuildFileTree(const char *RootUTF8)
{
    auto FileTree = dynamic_array<platform_file>(0);

    platform_file Root = {};
    Root.Parent = sml_u32(-1);
    strncpy_s(Root.FullPath, MaxNameLength, RootUTF8, _TRUNCATE);

    FileTree.Push(Root);

    for(sml_u32 Idx = 0; Idx < FileTree.Count; Idx++)
    {
        platform_file *Parent = &FileTree[Idx];
        if(!Parent->IsDir || Parent->Parent == sml_u32(-1))
        {
            continue;
        }

        wchar_t WidePath[MaxPathLength];
        SmlWin32_UTF8ToWide(Parent->FullPath, WidePath, MaxPathLength);

        WIN32_FIND_DATAW Data;
        HANDLE           Handle = INVALID_HANDLE_VALUE;
        if(Handle == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        do
        {
            if(wcscmp(Data.cFileName, L".") == 0 || wcscmp(Data.cFileName, L"..") == 0)
            {
                continue;
            }

            platform_file Entry = {};
            Entry.IsDir  = (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            Entry.Parent = Idx;
            SmlWin32_WideToUTF8(Data.cFileName, Entry.Name, MaxNameLength);
            snprintf(Entry.FullPath, MaxPathLength, "%s\\%ls",
                     Parent->FullPath, Data.cFileName);

            Parent->Children.Push(FileTree.Count);
            FileTree.Push(Entry);

        } while(FindNextFileW(Handle, &Data));

        FindClose(Handle);
    }

    return FileTree;
}
