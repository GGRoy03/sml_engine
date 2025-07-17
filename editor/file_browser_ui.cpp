// ===================================
// Type Definitions
// ===================================

struct directory_editor
{
    dynamic_array<platform_file> FileTree;

    sml_u32 ActiveFolderIdx;
    sml_u32 ActiveFileIdx;

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

    if(!File->IsDir)
    {
        Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool NodeOpen = TreeNodeEx(File->Name, Flags);
    bool Hovered  = IsItemHovered();

    if (Hovered && IsMouseDoubleClicked(0))
    {
        if (File->IsDir)
        {
            bool IsSame = Idx == Editor->ActiveFolderIdx;

            if (!IsSame)
            {
                Editor->Backward.Push(Editor->ActiveFolderIdx);
                Editor->Forward.Reset();

                Editor->ActiveFolderIdx = Idx;
            }
        }
        else
        {
            Editor->ActiveFileIdx = Idx;
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

        Editor->ActiveFolderIdx = 0;
        Editor->ActiveFileIdx   = sml_u32(-1);
    }

    if (Begin("File Browser", nullptr, 0))
    {
        if (Button("<--"))
        {
            if (!Editor->Backward.Empty())
            {
                Editor->Forward.Push(Editor->ActiveFolderIdx);
                Editor->ActiveFolderIdx = Editor->Backward.Pop();
            }
        }
        SameLine();
        if (Button("-->"))
        {
            if (!Editor->Forward.Empty())
            {
                Editor->Backward.Push(Editor->ActiveFolderIdx);
                Editor->ActiveFolderIdx = Editor->Forward.Pop();
            }
        }

        Spacing();

        if(Editor->Backward.Count > 0)
        {
            sml_u32 NodeIdx = Editor->Backward.Under();
            sml_u32 IterIdx = 0;
            while(IterIdx < Editor->Backward.Count)
            {
                auto *Folder = Editor->FileTree.Values + NodeIdx;

                if(Button(Folder->Name))
                {

                }

                SameLine(0, 0);
                TextUnformatted(" > ");
                SameLine(0, 0);

                NodeIdx = Editor->Backward.Values[++IterIdx];
            }

            auto* Folder = Editor->FileTree.Values + Editor->ActiveFolderIdx;
            if (Button(Folder->Name))
            {

            }
        }
        else
        {
            auto* Folder = Editor->FileTree.Values + Editor->ActiveFolderIdx;
            if (Button(Folder->Name))
            {

            }
        }

        Spacing();
        Separator();
        Spacing();

        if (Editor->FileTree.Count > 0)
        {
            platform_file Root = Editor->FileTree[Editor->ActiveFolderIdx];
            for (sml_u32 Idx = 0; Idx < Root.Children.Count; Idx++)
            {
                SmlIntEditor_DrawEntry(Editor, Root.Children[Idx]);
            }
        }
    }
    End();
}
