struct mesh_editor
{
    mesh_record *ActiveMesh;
    bool Visible;
};

#include "file_browser_ui.cpp"
#include "mesh_editor_ui.cpp"

struct editor
{
    file_browser_ui FileBrowser;

    bool IsInitialized;
};

static ImVec4
HexColor(sml_u32 Hex, sml_f32 Alpha = 1.0f) 
{
    sml_f32 R = ((Hex >> 16) & 0xFF) / 255.0f;
    sml_f32 G = ((Hex >>  8) & 0xFF) / 255.0f;
    sml_f32 B = ((Hex      ) & 0xFF) / 255.0f;

    return ImVec4(R, G, B, Alpha);
}

static void
SmlIntEditor_SetMainTheme()
{
    ImGuiStyle& Style = ImGui::GetStyle();
    ImVec4* c = Style.Colors;

    c[ImGuiCol_WindowBg]       = HexColor(0x212121);
    c[ImGuiCol_ChildBg]        = HexColor(0x2E2E3A);
    c[ImGuiCol_FrameBg]        = HexColor(0x2E2E3A);
    c[ImGuiCol_FrameBgHovered] = HexColor(0x383842);
    c[ImGuiCol_FrameBgActive]  = HexColor(0x41414D);
    c[ImGuiCol_Header]         = HexColor(0x383842);
    c[ImGuiCol_HeaderHovered]  = HexColor(0x444450);
    c[ImGuiCol_HeaderActive]   = HexColor(0x4E4E5A);
    c[ImGuiCol_Text]           = HexColor(0xE0E0E0);
    c[ImGuiCol_TextDisabled]   = HexColor(0x9E9E9E);

    const ImVec4 Purple = HexColor(0x7C4DFF);
    c[ImGuiCol_Button]        = Purple;
    c[ImGuiCol_ButtonHovered] = HexColor(0x8E65FF);
    c[ImGuiCol_ButtonActive]  = HexColor(0x6A3BFF);
    c[ImGuiCol_Separator]     = Purple;
    c[ImGuiCol_ResizeGrip]    = Purple;

    const ImVec4 Orange = HexColor(0xFF8F00);
    c[ImGuiCol_PlotHistogram]        = Orange;
    c[ImGuiCol_PlotHistogramHovered] = HexColor(0xFF9F20);

    const ImVec4 Blue = HexColor(0x448AFF);
    c[ImGuiCol_PlotLines]        = Blue;
    c[ImGuiCol_PlotLinesHovered] = HexColor(0x5AAFFF);
    c[ImGuiCol_SliderGrab]       = Blue;
    c[ImGuiCol_SliderGrabActive] = HexColor(0x3A76E6);

    Style.WindowRounding = 4.0f;
    Style.FrameRounding  = 3.0f;
    Style.GrabRounding   = 3.0f;
}

static void
SmlEditor_Render()
{
    static editor Editor;

    if(!Editor.IsInitialized)
    {
        SmlIntEditor_SetMainTheme();

        Editor.FileBrowser.Visible = true;
    }

    if(Editor.FileBrowser.Visible)
    {
        SmlEditor_FileBrowser(&Editor.FileBrowser);
    }
}
