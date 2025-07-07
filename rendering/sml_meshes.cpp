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

// NOTE: If GetCubeMesh works, remove this.
static sml_vertex CubeVertices[] =
{
    // Front face (Z+)
    {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},

    // Back face (Z-)
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},

    // Left face (X-)
    {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},

    // Right face (X+)
    {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},

    // Top face (Y+)
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},

    // Bottom face (Y-)
    {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
};

// NOTE: If GetCubeMesh works, remove this.
static u32 CubeIndices[] =
{
    // Front face
    0, 1, 2,   0, 2, 3,
    // Back face
    4, 5, 6,   4, 6, 7,
    // Left face
    8, 9, 10,  8, 10, 11,
    // Right face
    12, 13, 14, 12, 14, 15,
    // Top face
    16, 17, 18, 16, 18, 19,
    // Bottom face
    20, 21, 22, 20, 22, 23
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
    }

    inline sml_dynamic_array<sml_vector3> PackPositions()
    {
        auto Positions = 
            sml_dynamic_array<sml_vector3>(sml_u32(this->VtxHeap.Size / sizeof(V)));

        for(sml_u32 Idx = 0; Idx < Positions.Capacity; Idx++)
        {
            Positions[Idx] = this->VtxData[Idx].Position;
        }

        return Positions;
    } 

    inline sml_u32 IndexCount()
    {
        return sml_u32(this->IdxHeap.Size / sizeof(I));
    }
};

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
