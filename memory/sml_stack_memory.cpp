// ===================================
// Type Definitions
// ===================================

struct sml_memory
{
    void  *PushBase;
    size_t PushSize;
    size_t PushCapacity;
};

static constexpr size_t SmlDefaultMemorySize = Sml_Kilobytes(50);

// ===================================
// Global Variables
// ===================================

static sml_memory SmlMemory;

// ===================================
// Internal Helpers
// ===================================

static size_t
SmlInt_BeginTemporaryRegion()
{
    return SmlMemory.PushSize;
}

static void
SmlInt_EndTemporaryRegion( size_t Marker)
{
    if(SmlMemory.PushSize + Marker > SmlMemory.PushCapacity)
    {
        Sml_Assert("Memory overflow from temporary region.");
    }
}

static void*
SmlInt_PushMemory(size_t Size)
{
    if(!SmlMemory.PushBase)
    {
        SmlMemory.PushBase     = malloc(SmlDefaultMemorySize);
        SmlMemory.PushCapacity = SmlDefaultMemorySize;
        SmlMemory.PushSize     = 0;
    }

    void *Block         = (sml_u8*)SmlMemory.PushBase + SmlMemory.PushSize;
    SmlMemory.PushSize += Size;

    return Block;
}
