// ===================================
// Type Definitions
// ===================================

struct directory_editor
{
    dynamic_array<platform_file> FileTree;
    dynamic_array<bool>          Expanded;

    sml_u32 ActiveIdx;

    stack<sml_u32> Forward;
    stack<sml_u32> Backward;

    bool Visible;
};

// ===================================
// Internal Helpers
// ===================================

static void
SmlIntEditor_DrawEntry(directory_editor *Editor, sml_u32 Idx)
{
    using namespace ImGui;

    platform_file *File = Editor->FileTree.Values + Idx;

    ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow;

    if(Editor->Expanded[Idx])
    {
        Flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if(!File->IsDir)
    {
        Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool NodeOpen = TreeNodeEx(File->Name, Flags);

    if(IsItemClicked(0))
    {
        if(File->IsDir)
        {
            bool IsSame = Idx == Editor->ActiveIdx;

            if(!IsSame)
            {
                Editor->Backward.Push(Editor->ActiveIdx);
                Editor->Forward.Reset();
                Editor->ActiveIdx = Idx;
            }

            Editor->Expanded[Idx] = !Editor->Expanded[Idx];
        }
        else
        {
            // TODO: File selection
        }
    }

    if(File->IsDir && NodeOpen)
    {
        for (sml_u32 Child = 0; Child < File->Children.Count; Child++)
        {
            SmlIntEditor_DrawEntry(Editor, File->Children[Child]);
        }

        TreePop();
    }
}


// ===================================
// UI Components
// ===================================

static void
SmlEditor_FileBrowser(directory_editor *Editor)
{
    using namespace ImGui;

    if (!Editor->FileTree.Values)
    {
        const char *Root = "D:/Work/demo/small_engine";
        Editor->FileTree = Platform_BuildFileTree(Root);
        Editor->Forward  = stack<sml_u32>(10);
        Editor->Backward = stack<sml_u32>(10);
        Editor->Expanded = dynamic_array<bool>(Editor->FileTree.Count);
        Editor->ActiveIdx = 0;
    }

    if (Begin("File Browser", nullptr, 0))
    {
        if (Button("<--"))
        {
            if (!Editor->Backward.Empty())
            {
                Editor->Forward.Push(Editor->ActiveIdx);
                Editor->ActiveIdx  = Editor->Backward.Pop();
            }
        }
        SameLine();
        if (Button("-->"))
        {
            if (!Editor->Forward.Empty())
            {
                Editor->Backward.Push(Editor->ActiveIdx);
                Editor->ActiveIdx = Editor->Forward.Pop();
            }
        }
        SameLine();
        if (Button("Up"))
        {
        }

        Separator();

        if (Editor->FileTree.Count > 0)
        {
            platform_file Root = Editor->FileTree[Editor->ActiveIdx];
            for (sml_u32 Idx = 0; Idx < Root.Children.Count; Idx++)
            {
                SmlIntEditor_DrawEntry(Editor, Root.Children[Idx]);
            }
        }
    }
    End();
}
