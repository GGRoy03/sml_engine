// ===================================
// Internal Helpers
// ===================================

static void
FormatToByteUnits(sml_f64 NumBytes, char *OutBuffer, sml_u32 OutBufferSize)
{
    char    Unit[16] = {};
    sml_f64 Display  = NumBytes;

    if (Display > Sml_Megabytes(1))
    {
        Display /= Sml_Megabytes(1);
        memcpy(Unit, "Megabytes", 10);
    }
    else if (Display > Sml_Kilobytes(1))
    {
        Display /= Sml_Kilobytes(1);
        memcpy(Unit, "Kylobytes", 10);
    }
    else
    {
        memcpy(Unit, "Bytes", 6);
    }

    snprintf(OutBuffer, OutBufferSize, "%.2f %s", Display, Unit);
}

// ===================================
// UI Components
// ===================================

static void
SmlEditor_MeshUI(mesh_editor *Editor)
{
    using namespace ImGui;

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse;

    PushStyleColor(ImGuiCol_WindowBg      , IM_COL32(20 , 20 , 20 , 255));
    PushStyleColor(ImGuiCol_Header        , IM_COL32(255, 140, 0  , 200));
    PushStyleColor(ImGuiCol_HeaderHovered , IM_COL32(255, 160, 0  , 255));
    PushStyleColor(ImGuiCol_FrameBg       , IM_COL32(30 , 30 , 30 , 255));
    PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(255, 140, 0  , 100));
    PushStyleColor(ImGuiCol_Text          , IM_COL32(255, 255, 255, 255));
    PushStyleColor(ImGuiCol_Border        , IM_COL32(255, 140, 0  , 100));
    PushStyleColor(ImGuiCol_Separator     , IM_COL32(255, 140, 0  , 100));

    Begin("Meshes Manager", nullptr, Flags);

        const sml_f32 LeftWidth = 250.0f;
        BeginChild("##MeshListPanel", ImVec2(LeftWidth, 0), true);
            Text("Meshes");
            Separator();

            BeginChild("##MeshListScroll", ImVec2(0, 275), false);
                u32 Idx = SmlMeshes.Head;
                while (Idx != slot_map<mesh_record, u32>::Invalid)
                {
                    auto* Rec = SmlMeshes.Data + Idx;
                    if (Selectable(Rec->Name, false))
                    {
                        Editor->ActiveMesh = SmlMeshes.Data + Idx;
                    }
                    Idx = SmlMeshes.Next[Idx];
                }
            EndChild();

            Spacing();
            Separator();
            Text("Mesh Inforation");

            BeginChild("##MeshMetadata", ImVec2(0, 0));
                ImGuiTableFlags MetaDataFlags = ImGuiTableFlags_Borders;
                if(Editor->ActiveMesh && BeginTable("Mesh Information", 2, MetaDataFlags))
                {
                    auto *Act = Editor->ActiveMesh;

                    TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 125.0f);
                    TableSetupColumn("Value"   , ImGuiTableColumnFlags_WidthStretch);

                    TableNextRow();
                    TableSetColumnIndex(0);
                        Text("Name");
                    TableSetColumnIndex(1);
                        Text(Act->Name);

                    TableNextRow();
                    TableSetColumnIndex(0);
                        Text("Vertex Data Size");
                    TableSetColumnIndex(1);
                        char VtxByte[32] = {};
                        FormatToByteUnits(sml_f64(Act->VtxHeap.Size), VtxByte, 32);
                        Text(VtxByte);

                    TableNextRow();
                    TableSetColumnIndex(0);
                        Text("Index  Data Size");
                    TableSetColumnIndex(1);
                        char IdxByte[32] = {};
                        FormatToByteUnits(sml_f64(Act->IdxHeap.Size), IdxByte, 32);
                        Text(IdxByte);

                    ImGui::EndTable();
                }
            EndChild();
        EndChild();
    End();
    PopStyleColor(8);
}
