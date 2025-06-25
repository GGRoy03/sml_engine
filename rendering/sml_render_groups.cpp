// ===================================
//  Type Definitions
// ===================================

struct sml_static_group
{
    sml_u32 Handle;
    void   *BackendData;
};

// ===================================
//  User API
// ===================================

// NOTE: I hate this API

static sml_static_group 
SmlDx11_BeginStaticGroup(sml_pipeline_desc Desc)
{
    switch(ActiveBackend)
    {

#ifdef _WIN32

    case SmlRenderer_DX11:
    {
        return SmlDx11_BeginStaticGroup(Desc);
    } break;

#endif

    }

#ifdef _WIN32
#endif
}
