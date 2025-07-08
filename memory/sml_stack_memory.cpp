// ===================================
// Type Definitions
// ===================================

// NOTE: Generation counter on blocks for easier debugging?
// I think this way of doing things can easily go out of control.

struct sml_memory_block
{
    void  *Data;
    size_t At;
    size_t Size;
};

// NOTE: OOP?
struct sml_memory
{
    void  *PushBase;
    size_t PushSize;
    size_t PushCapacity;

    sml_memory_block *FreeList;
    sml_u32           FreeCount;
};

static constexpr size_t  SmlMemorySize   = Sml_Kilobytes(50);
static constexpr sml_u32 SmlFreeListSize = 10;

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
SmlInt_EndTemporaryRegion(size_t Marker)
{
    if(SmlMemory.PushSize + Marker > SmlMemory.PushCapacity)
    {
        Sml_Assert(!"Memory overflow from temporary region.");
    }

    SmlMemory.PushSize = Marker;
}

// BUG: Underflow bug somewhere?

static sml_memory_block
SmlInt_PushMemory(size_t Size)
{
    if(!SmlMemory.PushBase)
    {
        SmlMemory.PushBase     = malloc(SmlMemorySize);
        SmlMemory.PushCapacity = SmlMemorySize;
        SmlMemory.PushSize     = 0;
        SmlMemory.FreeCount    = 0;
        SmlMemory.FreeList     = 
            (sml_memory_block*)malloc(SmlFreeListSize * sizeof(sml_memory_block));
    }

    if(SmlMemory.PushSize + Size > SmlMemory.PushCapacity)
    {
        Sml_Assert(!"Out of memory");
    }

    for(sml_u32 FreeIndex = SmlMemory.FreeCount; FreeIndex-- > 0;)
    {
        sml_memory_block *FreeBlock = SmlMemory.FreeList + FreeIndex;

        if(FreeBlock->Size >= Size)
        {
            size_t OrigAt   = FreeBlock->At;
            size_t OrigSize = FreeBlock->Size;

            auto Last  = SmlMemory.FreeList[--SmlMemory.FreeCount];
            *FreeBlock = Last;

            sml_memory_block Result = {};
            Result.Data = (sml_u8*)SmlMemory.PushBase + FreeBlock->At;
            Result.Size = Size;
            Result.At   = OrigAt;

            size_t Diff = OrigSize - Size;
            if(Diff > 0)
            {
                sml_memory_block Leftover = {};
                Leftover.Data = (sml_u8*)SmlMemory.PushBase + Result.At + Size;
                Leftover.Size = Diff;
                Leftover.At   = Result.At + Size;

                Sml_Assert(SmlMemory.FreeCount <= SmlFreeListSize);
                SmlMemory.FreeList[SmlMemory.FreeCount++] = Leftover;
            }

            return Result;
        }
    }

    sml_memory_block Block = {};
    Block.Data = (sml_u8*)SmlMemory.PushBase + SmlMemory.PushSize;
    Block.Size = Size;
    Block.At   = SmlMemory.PushSize;

    SmlMemory.PushSize += Size;

    return Block;
}

static void
SmlInt_FreeMemory(sml_memory_block Block)
{
    if(SmlMemory.FreeCount == SmlFreeListSize)
    {
        Sml_Assert(!"Free list is already full");
        return;
    }

    SmlMemory.FreeList[SmlMemory.FreeCount++] = Block;
}

static sml_memory_block
SmlInt_ReallocateMemory(sml_memory_block OldBlock, sml_u32 Growth)
{
    sml_memory_block NewBlock = SmlInt_PushMemory(OldBlock.Size * Growth);
    memcpy(NewBlock.Data, OldBlock.Data, OldBlock.Size);
    SmlInt_FreeMemory(OldBlock);

    return NewBlock;
}
