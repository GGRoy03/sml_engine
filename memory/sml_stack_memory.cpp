// ===================================
// Type Definitions
// ===================================

struct sml_memory_block
{
    void  *Data;
    size_t At;
    size_t Size;
};

struct sml_memory
{
    void  *PushBase;
    size_t PushSize;
    size_t PushCapacity;

    sml_memory_block *FreeList;
    sml_u32           FreeCount;
};

static constexpr size_t  SmlMemorySize    = Sml_Kilobytes(50);
static constexpr sml_u32 SmlFreeListCount = 10;

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

// WARN:
// 1) The behavior of this allocator is to always give V >= Size. If V is allocated
// from a free block it might concatenate the old free-block with itself for
// simplicity. We should not do that and instead carve the block into two and insert
// one of them (SizeDifference) into the free list. It's not a bug per-say, but it's
// not great behavior.

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
            (sml_memory_block*)malloc(SmlFreeListCount * sizeof(sml_memory_block));
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
            size_t SizeDifference = FreeBlock->Size - Size;

            sml_memory_block Block = {};
            Block.Data = (sml_u8*)SmlMemory.PushBase + FreeBlock->At;
            Block.Size = Size + SizeDifference;
            Block.At   = FreeBlock->At;

            SmlMemory.PushSize  += Size;
            SmlMemory.FreeCount -= 1;

            return Block;
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
    if(SmlMemory.FreeCount == SmlFreeListCount)
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
