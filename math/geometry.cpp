//////////////////////////////////////////////////////
///                  2D GEOMETRY                   ///
//////////////////////////////////////////////////////

// ===================================
// Type Definitions
// ===================================

enum SmlTriangulate_Method
{
    SmlTriangulate_Fan,
};

struct sml_tri_idx
{
    sml_u32 A, B, C;
};


static sml_dynamic_array<sml_tri_idx>
SmlInt_Triangulate(sml_dynamic_array<sml_vector2> Polygon,
                   SmlTriangulate_Method Method)
{
    auto IdxList = sml_dynamic_array<sml_tri_idx>(Polygon.Count - 2);

    switch(Method)
    {

    case SmlTriangulate_Fan:
    {
        for(sml_u32 Idx = 1; Idx + 1 < Polygon.Count; Idx++)
        {
            sml_tri_idx Tri = {0, Idx, Idx + 1}; 
            IdxList.Push(Tri);
        }
    } break;

    default:
        Sml_Assert(!"Invalid triangulation method.");
        break;

    }

    return IdxList;
}
