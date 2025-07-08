// ===========================================
// Type Definitions
// ==========================================

enum class sml_mesh_id : sml_u32 {};

struct sml_vertex
{
    sml_vector3 Position;
    sml_vector3 Normal;
    sml_vector2 UV;

    sml_vertex(sml_vector3 _Position, sml_vector3 _Normal, sml_vector2 _UV)
    {
        this->Position = _Position;
        this->Normal   = _Normal;
        this->UV       = _UV;
    }
};

struct sml_vertex_color
{
    sml_vector3 Position;
    sml_vector3 Normal;
    sml_vector3 Color;
};

template <typename V, typename I>
struct sml_mesh
{
    sml_memory_block VtxHeap;
    sml_memory_block IdxHeap;

    V *VtxData;
    I *IdxData;

    sml_mesh(){};
    sml_mesh(sml_u32 VtxCount, sml_u32 IdxCount)
    {
        this->VtxHeap = SmlInt_PushMemory(VtxCount * sizeof(V));
        this->IdxHeap = SmlInt_PushMemory(IdxCount * sizeof(I));

        this->VtxData = (V*)this->VtxHeap.Data;
        this->IdxData = (I*)this->IdxHeap.Data;
    }

    inline sml_dynamic_array<sml_vector3> PackPositions()
    {
        auto Positions = 
            sml_dynamic_array<sml_vector3>(sml_u32(this->VtxHeap.Size / sizeof(V)));

        for(sml_u32 Idx = 0; Idx < Positions.Capacity; Idx++)
        {
            Positions.Push(this->VtxData[Idx].Position);
        }

        return Positions;
    } 

    inline sml_u32 VertexCount()
    {
        return sml_u32(this->VtxHeap.Size / sizeof(V));
    }

    inline sml_u32 IndexCount()
    {
        return sml_u32(this->IdxHeap.Size / sizeof(I));
    }

};

struct sml_mesh_record
{
    // Meta-data
    char Name[32];

    // Mesh-data
    sml_memory_block VtxHeap;
    sml_memory_block IdxHeap;
};

// ===========================================
// Global Variables
// ==========================================

// NOTE: Everything ends up being a global...
sml_slot_map<sml_mesh_record, sml_u32> SmlMeshes;

// ===========================================
// User API
// ==========================================

template <typename V, typename I>
static sml_mesh_id
Sml_RecordMesh(sml_mesh<V, I> Mesh, const char* Name)
{
    // NOTE: Not really sure.
    if(!SmlMeshes.Data && !SmlMeshes.FreeList)
    {
        SmlMeshes = sml_slot_map<sml_mesh_record, sml_u32>(10);
    }

    sml_mesh_record Record = {};
    Record.VtxHeap = Mesh.VtxHeap;
    Record.IdxHeap = Mesh.IdxHeap;

    size_t NameLength = strlen(Name);
    if(NameLength > 31) NameLength = 31;
    memcpy(Record.Name, Name, NameLength);

    sml_mesh_id Id = sml_mesh_id(SmlMeshes.Emplace(Record));

    return Id;
}

template <typename V, typename I>
static sml_mesh<V, I>
Sml_GetMesh(sml_mesh_id Id)
{
    sml_mesh_record Record = SmlMeshes[sml_u32(Id)];

    sml_mesh<V, I> Mesh = {};
    Mesh.VtxHeap = Record.VtxHeap;
    Mesh.IdxHeap = Record.IdxHeap;
    Mesh.VtxData = (V*)Mesh.VtxHeap.Data;
    Mesh.IdxData = (I*)Mesh.IdxHeap.Data;

    return Mesh;
}

static sml_mesh<sml_vertex, sml_u32>
Sml_GetCubeMesh()
{
    sml_mesh<sml_vertex, sml_u32> Mesh = {};
    Mesh.VtxHeap = SmlInt_PushMemory(24 * sizeof(sml_vertex));
    Mesh.IdxHeap = SmlInt_PushMemory(36 * sizeof(sml_u32));
    Mesh.VtxData = (sml_vertex*)Mesh.VtxHeap.Data;
    Mesh.IdxData = (sml_u32*)Mesh.IdxHeap.Data;

    sml_u32 V = 0;
    sml_u32 I = 0;

    // Front face
    Mesh.VtxData[V++] = sml_vertex({-0.5f,-0.5f,0.5f},{0.0f,0.0f,1.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f,-0.5f,0.5f},{0.0f,0.0f,1.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f, 0.5f,0.5f},{0.0f,0.0f,1.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f, 0.5f,0.5f},{0.0f,0.0f,1.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 0; Mesh.IdxData[I++] = 1; Mesh.IdxData[I++] = 2;
    Mesh.IdxData[I++] = 0; Mesh.IdxData[I++] = 2; Mesh.IdxData[I++] = 3;

    // Back face
    Mesh.VtxData[V++] = sml_vertex({ 0.5f,-0.5f,-0.5f},{0.0f,0.0f,-1.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f,-0.5f,-0.5f},{0.0f,0.0f,-1.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f, 0.5f,-0.5f},{0.0f,0.0f,-1.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f, 0.5f,-0.5f},{0.0f,0.0f,-1.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 4; Mesh.IdxData[I++] = 5; Mesh.IdxData[I++] = 6;
    Mesh.IdxData[I++] = 4; Mesh.IdxData[I++] = 6; Mesh.IdxData[I++] = 7;

    // Left face
    Mesh.VtxData[V++] = sml_vertex({-0.5f,-0.5f,-0.5f},{-1.0f,0.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f,-0.5f, 0.5f},{-1.0f,0.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f, 0.5f, 0.5f},{-1.0f,0.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f, 0.5f,-0.5f},{-1.0f,0.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 8;  Mesh.IdxData[I++] = 9;  Mesh.IdxData[I++] = 10;
    Mesh.IdxData[I++] = 8;  Mesh.IdxData[I++] = 10; Mesh.IdxData[I++] = 11;

    // Right face
    Mesh.VtxData[V++] = sml_vertex({0.5f,-0.5f, 0.5f},{1.0f,0.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({0.5f,-0.5f,-0.5f},{1.0f,0.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({0.5f, 0.5f,-0.5f},{1.0f,0.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = sml_vertex({0.5f, 0.5f, 0.5f},{1.0f,0.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 12; Mesh.IdxData[I++] = 13; Mesh.IdxData[I++] = 14;
    Mesh.IdxData[I++] = 12; Mesh.IdxData[I++] = 14; Mesh.IdxData[I++] = 15;

    // Top face
    Mesh.VtxData[V++] = sml_vertex({-0.5f,0.5f, 0.5f},{0.0f,1.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f,0.5f, 0.5f},{0.0f,1.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f,0.5f,-0.5f},{0.0f,1.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f,0.5f,-0.5f},{0.0f,1.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 16; Mesh.IdxData[I++] = 17; Mesh.IdxData[I++] = 18;
    Mesh.IdxData[I++] = 16; Mesh.IdxData[I++] = 18; Mesh.IdxData[I++] = 19;

    // Bottom face
    Mesh.VtxData[V++] = sml_vertex({-0.5f,-0.5f,-0.5f},{0.0f,-1.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f,-0.5f,-0.5f},{0.0f,-1.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = sml_vertex({ 0.5f,-0.5f, 0.5f},{0.0f,-1.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = sml_vertex({-0.5f,-0.5f, 0.5f},{0.0f,-1.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 20; Mesh.IdxData[I++] = 21; Mesh.IdxData[I++] = 22;
    Mesh.IdxData[I++] = 20; Mesh.IdxData[I++] = 22; Mesh.IdxData[I++] = 23;

    return Mesh;
}
