// ===========================================
// Type Definitions
// ==========================================

enum class mesh_id : sml_u32 {};

struct vertex
{
    sml_vector3 Position;
    sml_vector3 Normal;
    sml_vector2 UV;

    vertex(sml_vector3 _Position, sml_vector3 _Normal, sml_vector2 _UV)
    {
        this->Position = _Position;
        this->Normal   = _Normal;
        this->UV       = _UV;
    }
};

struct vertex_color
{
    sml_vector3 Position;
    sml_vector3 Normal;
    sml_vector3 Color;
};

template <typename V, typename I>
struct mesh
{
    sml_heap_block VtxHeap;
    sml_heap_block IdxHeap;

    V *VtxData;
    I *IdxData;

    mesh(){};
    mesh(sml_u32 VtxCount, sml_u32 IdxCount)
    {
        this->VtxHeap = SmlMemory.Allocate(VtxCount * sizeof(V));
        this->IdxHeap = SmlMemory.Allocate(IdxCount * sizeof(I));

        this->VtxData = (V*)this->VtxHeap.Data;
        this->IdxData = (I*)this->IdxHeap.Data;
    }

    inline dynamic_array<sml_vector3> PackPositions()
    {
        auto Positions = 
            dynamic_array<sml_vector3>(sml_u32(this->VtxHeap.Size / sizeof(V)));

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

struct mesh_record
{
    // Meta-data
    char Name[32];

    // Mesh-data
    sml_heap_block VtxHeap;
    sml_heap_block IdxHeap;
};

// ===========================================
// Global Variables
// ==========================================

static slot_map<mesh_record, sml_u32> SmlMeshes;

// ===========================================
// User API
// ==========================================

template <typename V, typename I>
static mesh_id
RecordMesh(mesh<V, I> Mesh, const char* Name)
{
    // WARN: Bad?
    if(!SmlMeshes.Data && !SmlMeshes.FreeList)
    {
        SmlMeshes = slot_map<mesh_record, sml_u32>(10);
    }

    mesh_record Record = {};
    Record.VtxHeap = Mesh.VtxHeap;
    Record.IdxHeap = Mesh.IdxHeap;

    size_t NameLength = strlen(Name);
    if(NameLength > 31) NameLength = 31;
    memcpy(Record.Name, Name, NameLength);

    mesh_id Id = mesh_id(SmlMeshes.Emplace(Record));

    return Id;
}

template <typename V, typename I>
static mesh<V, I>
GetMesh(mesh_id Id)
{
    mesh_record Record = SmlMeshes[sml_u32(Id)];

    mesh<V, I> Mesh = {};
    Mesh.VtxHeap = Record.VtxHeap;
    Mesh.IdxHeap = Record.IdxHeap;
    Mesh.VtxData = (V*)Mesh.VtxHeap.Data;
    Mesh.IdxData = (I*)Mesh.IdxHeap.Data;

    return Mesh;
}

static mesh<vertex, sml_u32>
GetCubeMesh()
{
    const sml_u32 VtxCount = 24;
    const sml_u32 IdxCount = 36;

    auto Mesh = mesh<vertex, sml_u32>(VtxCount, IdxCount);
    Mesh.VtxData = (vertex*)Mesh.VtxHeap.Data;
    Mesh.IdxData = (sml_u32*)Mesh.IdxHeap.Data;

    sml_u32 V = 0;
    sml_u32 I = 0;

    // Front face
    Mesh.VtxData[V++] = vertex({-0.5f,-0.5f,0.5f},{0.0f,0.0f,1.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f,-0.5f,0.5f},{0.0f,0.0f,1.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f, 0.5f,0.5f},{0.0f,0.0f,1.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = vertex({-0.5f, 0.5f,0.5f},{0.0f,0.0f,1.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 0; Mesh.IdxData[I++] = 1; Mesh.IdxData[I++] = 2;
    Mesh.IdxData[I++] = 0; Mesh.IdxData[I++] = 2; Mesh.IdxData[I++] = 3;

    // Back face
    Mesh.VtxData[V++] = vertex({ 0.5f,-0.5f,-0.5f},{0.0f,0.0f,-1.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = vertex({-0.5f,-0.5f,-0.5f},{0.0f,0.0f,-1.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = vertex({-0.5f, 0.5f,-0.5f},{0.0f,0.0f,-1.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f, 0.5f,-0.5f},{0.0f,0.0f,-1.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 4; Mesh.IdxData[I++] = 5; Mesh.IdxData[I++] = 6;
    Mesh.IdxData[I++] = 4; Mesh.IdxData[I++] = 6; Mesh.IdxData[I++] = 7;

    // Left face
    Mesh.VtxData[V++] = vertex({-0.5f,-0.5f,-0.5f},{-1.0f,0.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = vertex({-0.5f,-0.5f, 0.5f},{-1.0f,0.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = vertex({-0.5f, 0.5f, 0.5f},{-1.0f,0.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = vertex({-0.5f, 0.5f,-0.5f},{-1.0f,0.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 8;  Mesh.IdxData[I++] = 9;  Mesh.IdxData[I++] = 10;
    Mesh.IdxData[I++] = 8;  Mesh.IdxData[I++] = 10; Mesh.IdxData[I++] = 11;

    // Right face
    Mesh.VtxData[V++] = vertex({0.5f,-0.5f, 0.5f},{1.0f,0.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = vertex({0.5f,-0.5f,-0.5f},{1.0f,0.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = vertex({0.5f, 0.5f,-0.5f},{1.0f,0.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = vertex({0.5f, 0.5f, 0.5f},{1.0f,0.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 12; Mesh.IdxData[I++] = 13; Mesh.IdxData[I++] = 14;
    Mesh.IdxData[I++] = 12; Mesh.IdxData[I++] = 14; Mesh.IdxData[I++] = 15;

    // Top face
    Mesh.VtxData[V++] = vertex({-0.5f,0.5f, 0.5f},{0.0f,1.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f,0.5f, 0.5f},{0.0f,1.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f,0.5f,-0.5f},{0.0f,1.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = vertex({-0.5f,0.5f,-0.5f},{0.0f,1.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 16; Mesh.IdxData[I++] = 17; Mesh.IdxData[I++] = 18;
    Mesh.IdxData[I++] = 16; Mesh.IdxData[I++] = 18; Mesh.IdxData[I++] = 19;

    // Bottom face
    Mesh.VtxData[V++] = vertex({-0.5f,-0.5f,-0.5f},{0.0f,-1.0f,0.0f},{0.0f,1.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f,-0.5f,-0.5f},{0.0f,-1.0f,0.0f},{1.0f,1.0f});
    Mesh.VtxData[V++] = vertex({ 0.5f,-0.5f, 0.5f},{0.0f,-1.0f,0.0f},{1.0f,0.0f});
    Mesh.VtxData[V++] = vertex({-0.5f,-0.5f, 0.5f},{0.0f,-1.0f,0.0f},{0.0f,0.0f});
    Mesh.IdxData[I++] = 20; Mesh.IdxData[I++] = 21; Mesh.IdxData[I++] = 22;
    Mesh.IdxData[I++] = 20; Mesh.IdxData[I++] = 22; Mesh.IdxData[I++] = 23;

    return Mesh;
}
