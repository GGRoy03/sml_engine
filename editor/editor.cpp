// ===================================
// Type Definitions
// ===================================

struct directory_editor
{
    dynamic_array<platform_file> FileTree;
};

struct mesh_editor
{
    mesh_record *ActiveMesh;
    bool Visible;
};

struct editor
{
    mesh_editor MeshEditor;
};

#include "mesh_editor_ui.cpp"

static void
SmlEditor_Render()
{
    static editor Editor;
    Editor.MeshEditor.Visible = true;

    if(Editor.MeshEditor.Visible)
    {
        SmlEditor_MeshUI(&Editor.MeshEditor);
    }
}
