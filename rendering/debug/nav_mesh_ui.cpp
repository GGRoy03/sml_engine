// ===================================
// Type Definitions
// ===================================

struct navmesh_debug_manager
{
    mesh_record *ActiveMesh;

    bool IsInitialized;
};

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
// User API
// ===================================

static void 
DebugNavmesh()
{
    static navmesh_debug_manager Manager;

    if (!Manager.IsInitialized)
    {
        Manager.ActiveMesh = nullptr;

        Manager.IsInitialized = true;
    }

    ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg      , IM_COL32(20 , 20 , 20 , 255));
    ImGui::PushStyleColor(ImGuiCol_Header        , IM_COL32(255, 140, 0  , 200));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered , IM_COL32(255, 160, 0  , 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg       , IM_COL32(30 , 30 , 30 , 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(255, 140, 0  , 100));
    ImGui::PushStyleColor(ImGuiCol_Text          , IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Border        , IM_COL32(255, 140, 0  , 100));
    ImGui::PushStyleColor(ImGuiCol_Separator     , IM_COL32(255, 140, 0  , 100));

    ImGui::Begin("Nav‑Mesh Builder##NavMeshUI", nullptr, Flags);

    sml_f32 LeftWidth = 250.0f;
    ImGui::BeginChild("##MeshListPanel", ImVec2(LeftWidth, 0), true);
    ImGui::Text("Meshes");
    ImGui::Separator();

    ImGui::BeginChild("##MeshListScroll", ImVec2(0, 275), false);
    u32 Idx = SmlMeshes.Head;
    while (Idx != slot_map<mesh_record, u32>::Invalid)
    {
        auto* Rec = SmlMeshes.Data + Idx;
        if (ImGui::Selectable(Rec->Name, false))
        {
            Manager.ActiveMesh = SmlMeshes.Data + Idx;
        }
        Idx = SmlMeshes.Next[Idx];
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Mesh Metadata");
    ImGui::BeginChild("##MeshMetaLeft", ImVec2(0, 0), false);

    ImGuiTableFlags MetaDataFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if (Manager.ActiveMesh && ImGui::BeginTable("MeshMetadata", 2, MetaDataFlags))
    {
        auto *Act = Manager.ActiveMesh;

        ImGui::TableSetupColumn("Property",ImGuiTableColumnFlags_WidthFixed, 125.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
            ImGui::Text("Name");
        ImGui::TableSetColumnIndex(1);
            ImGui::Text(Act->Name);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
            ImGui::Text("Vertex Data Size");
        ImGui::TableSetColumnIndex(1);
            char VtxByte[32] = {};
            FormatToByteUnits(sml_f64(Act->VtxHeap.Size), VtxByte, 32);
            ImGui::Text(VtxByte);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
            ImGui::Text("Index  Data Size");
        ImGui::TableSetColumnIndex(1);
            char IdxByte[32] = {};
            FormatToByteUnits(sml_f64(Act->IdxHeap.Size), IdxByte, 32);
            ImGui::Text(IdxByte);

        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##GroupAndMetaPanel", ImVec2(0, 0), true);

    ImGui::Text("Groups");
    ImGui::BeginChild("##GroupBuilder", ImVec2(0, 200), true);
    ImGui::TextWrapped("Drag meshes here to form a group");
    ImGui::EndChild();

    ImGui::Spacing();

    ImGui::Text("Group Metadata");
    ImGui::Separator();
    ImGui::BeginChild("##GroupMeta", ImVec2(0, 0), false);
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor(8);
}
