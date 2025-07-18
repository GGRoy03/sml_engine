// ===================================
// Type Definitions
// ===================================

enum FileBrowser_Flags : sml_u32
{
    // File Dragging States
    FileBrowser_DraggingFile = 1 << 0, 
    FileBrowser_DroppingFile = 1 << 1,
};

constexpr sml_bit_field FileBrowser_TransientMask =
    FileBrowser_DraggingFile |
    FileBrowser_DroppingFile
;

struct file_browser_ui
{
    dynamic_array<platform_file> FileTree;

    sml_u32 ActiveFolderIdx;
    sml_u32 ActiveFileIdx;

    stack<sml_u32> Forward;
    stack<sml_u32> Backward;

    // Options
    bool ShowHidden;
    bool SortByName;

    sml_bit_field State;

    bool Visible;
};

// ===================================
// Internal Helpers
// ===================================

static void
SmlIntEditor_DrawEntry(file_browser_ui *Editor, sml_u32 Idx)
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

    if(BeginDragDropSource())
    {
        SetDragDropPayload("PAYLOAD_FILE_ID", &Idx, sizeof(Idx));
        EndDragDropSource();

        Editor->State |= FileBrowser_DraggingFile;
    }
    else if(Editor->State & FileBrowser_DraggingFile)
    {
        Editor->State |=  FileBrowser_DroppingFile;
        Editor->State &= ~FileBrowser_DroppingFile;
    }

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

static void
SmlIntEditor_HandleClickOnHistory(file_browser_ui *Editor, sml_u32 ClickedIdx)
{
    if(Editor->Backward.Count > 0)
    {
        while(ClickedIdx != Editor->Backward.Peek())
        {
            Editor->Backward.Pop();
        }

        Editor->Backward.Pop();
        Editor->ActiveFolderIdx = ClickedIdx;
    }
}

// BUG: Does not work?

static inline void 
SmlIntEditor_FullWidthSeparator(sml_f32 WindowPadding)
{
    using namespace ImGui;

    Spacing();
    Indent(-WindowPadding);
    Separator();
    Unindent(-WindowPadding);
    Spacing();
}

// =================================== 
// UI Components
// ===================================

// WARN:
// We do some manual state tracking, because I haven't found a way to track whether
// or not we are dragging the source or not. We can check if we are dropping whatever
// it is we are holding, but then it's inconsistent with the code. Maybe the state
// flags approach is incorrect, it seems decent enough to me. My only worry is that
// one of the selling points of ImGui is that there is less state tracking.
// I mean, we gotta have some, that's just UI work?

// TODO: 
// 1) Dropping logic (Set the correct state, do the platform work, render correct UI)
// 2) Fix separator code
// 3) Implement sorting/hidden files
// 4) Lazy loading for the tree / LRU cache.

static void
SmlEditor_FileBrowser(file_browser_ui *Editor)
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
        sml_f32 WindowPadding = GetStyle().WindowPadding.x;

        sml_f32 BackwardAlpha = Editor->Backward.Empty() ? 0.5f : 1.0f;
        PushStyleVar(ImGuiStyleVar_Alpha, BackwardAlpha);
        if (Button("<--"))
        {
            if (!Editor->Backward.Empty())
            {
                Editor->Forward.Push(Editor->ActiveFolderIdx);
                Editor->ActiveFolderIdx = Editor->Backward.Pop();
            }
        }
        PopStyleVar(); SameLine();

        sml_f32 ForwardAlpha = Editor->Forward.Empty() ? 0.5f : 1.0f;
        PushStyleVar(ImGuiStyleVar_Alpha, ForwardAlpha);
        if (Button("-->"))
        {
            if (!Editor->Forward.Empty())
            {
                Editor->Backward.Push(Editor->ActiveFolderIdx);
                Editor->ActiveFolderIdx = Editor->Forward.Pop();
            }
        }
        PopStyleVar(); SameLine();

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
                    SmlIntEditor_HandleClickOnHistory(Editor, NodeIdx);
                }

                SameLine(0, 0);
                TextUnformatted(" > ");
                SameLine(0, 0);

                NodeIdx = Editor->Backward.Values[++IterIdx];
            }

            auto* Folder = Editor->FileTree.Values + Editor->ActiveFolderIdx;
            if (Button(Folder->Name))
            {
                SmlIntEditor_HandleClickOnHistory(Editor, NodeIdx);
            }
        }
        else
        {
            auto  Idx    = Editor->ActiveFolderIdx;
            auto* Folder = Editor->FileTree.Values + Idx;
            if (Button(Folder->Name))
            {
                SmlIntEditor_HandleClickOnHistory(Editor, Idx);
            }
        }

        SmlIntEditor_FullWidthSeparator(-WindowPadding);

        if (Editor->FileTree.Count > 0)
        {
            platform_file Root = Editor->FileTree[Editor->ActiveFolderIdx];
            for (sml_u32 Idx = 0; Idx < Root.Children.Count; Idx++)
            {
                SmlIntEditor_DrawEntry(Editor, Root.Children[Idx]);
            }
        }

        sml_f32 FooterHeight = GetFrameHeightWithSpacing() * 4;
        SetCursorPosY(GetWindowHeight() - FooterHeight);

        SmlIntEditor_FullWidthSeparator(-WindowPadding);

        if (FileBrowser_DraggingFile || FileBrowser_DroppingFile)
        {
            InvisibleButton("##File_Browser_Drop_Target", ImVec2(200, 200));

            if(BeginDragDropTarget())
            {
                if(const auto *Payload = AcceptDragDropPayload("PAYLOAD_FILE_ID"))
                {
                    IM_ASSERT(Payload->DataSize == sizeof(sml_u32));

                    sml_u32 PayloadIdx = *(sml_u32*)Payload->Data;
                }

                Editor->State &= ~FileBrowser_DroppingFile;

                EndDragDropTarget();
            }
        }
        else
        {
            if (Checkbox("Show Hidden", &Editor->ShowHidden))
            {
            }

            if (Checkbox("Sort By Name", &Editor->SortByName))
            {
            }
        }
    }

    End();
}
