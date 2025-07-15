// ===================================
// Type Definitions
// ===================================

struct directory_editor
{
    dynamic_array<platform_file> FileTree;

    bool Visible;
};

// ===================================
// Internal Helpers
// ===================================

// NOTE: Since file systems tend to be big, we might overflow the stack.
// We could use our own stack (really cheap)

static void
SmlIntEditor_DrawEntry(directory_editor *Editor, sml_u32 Idx)
{
    using namespace ImGui;

    platform_file *File = Editor->FileTree.Values + Idx;

    PushID(Idx);

    if(File->IsDir)
    {
        if(TreeNode(File->Name))
        {
            for(sml_u32 Child = 0; Child < File->Children.Count; Child++)
            {
                SmlIntEditor_DrawEntry(Editor, File->Children[Child]);
            }

            TreePop();
        }
    }
    else
    {
        Bullet(); SameLine();
        if(Selectable(File->Name))
        {
        }
    }

    PopID();
}

// ===================================
// UI Components
// ===================================

static void
SmlEditor_FileBrowser(directory_editor *Editor)
{
    using namespace ImGui;

    if(!Editor->FileTree.Values)
    {
        Editor->FileTree = Platform_BuildFileTree("../../small_engine");
    }

    if(Begin("File Browser", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar))
    {
        if (Editor->FileTree.Count > 0)
        {
            SmlIntEditor_DrawEntry(Editor, 0);
        }

        Text("Underneath");

        End();
    }
}
