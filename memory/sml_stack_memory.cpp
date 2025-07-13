// TODO: Simplify this code | Add coalescing

struct sml_heap_block
{
    void   *Data;
    size_t  At;
    size_t  Size;
    sml_u32 IntIdx;
};

struct sml_memory
{
    // Core-data
    void  *PushBase;
    size_t PushSize;
    size_t PushCapacity;

    // Free-list
    sml_heap_block *FreeList;
    sml_u32         FreeCount;

    sml_u32 *IdxStack;
    sml_u32  IdxStackCount;

    sml_u32 *NextArray;
    sml_u32 *PrevArray;
    sml_u32  Head;
    sml_u32  Tail;

    size_t BiggestFreeBlock;

    // Meta-data
    bool ResizeOnFull;

    static constexpr sml_u32 Invalid      = sml_u32(-1);
    static constexpr sml_u32 FreeListSize = 100;

    sml_memory(){};
    sml_memory(size_t HeapSize, bool ResizeOnFull = false)
    {
        this->PushBase     = malloc(HeapSize);
        this->PushSize     = 0;
        this->PushCapacity = HeapSize;

        this->FreeCount = 0;
        this->FreeList  =
            (sml_heap_block*)malloc(this->FreeListSize * sizeof(sml_heap_block));

        this->IdxStackCount = 0;
        this->IdxStack      = (sml_u32*)malloc(this->FreeListSize * sizeof(sml_u32));

        this->NextArray = (sml_u32*)malloc(this->FreeListSize * sizeof(sml_u32) * 2);
        this->PrevArray = this->NextArray + this->FreeListSize;
        this->Head      = this->Invalid;
        this->Tail      = this->Invalid;

        this->ResizeOnFull = ResizeOnFull;

        memset(this->PushBase , 0, HeapSize);
        memset(this->FreeList , 0, this->FreeListSize * sizeof(sml_heap_block));
        memset(this->IdxStack , 0, this->FreeListSize * sizeof(sml_u32));
        memset(this->NextArray, this->Invalid, this->FreeListSize * sizeof(sml_u32));
        memset(this->PrevArray, this->Invalid, this->FreeListSize * sizeof(sml_u32));
    }

    sml_heap_block Allocate(size_t RequestedSize)
    {
        Sml_Assert(this->PushBase  && this->FreeList &&
                   this->NextArray && this->PrevArray);

        if(this->PushSize + RequestedSize > this->PushCapacity)
        {
            if(this->ResizeOnFull)
            {
                // BUG: Missing logic.
            }
            else
            {
                Sml_Assert(!"Out of memory.");
            }
        }

        if(this->Tail != this->Invalid && RequestedSize <= this->BiggestFreeBlock)
        { 
            sml_heap_block *FreeBlock = nullptr;
            sml_u32         Idx       = this->Head;

            while(Idx != this->Invalid)
            {
                sml_heap_block *Block = this->FreeList + Idx;

                if(Block->Size >= RequestedSize)
                {
                    FreeBlock = Block;
                    break;
                }

                Idx = this->NextArray[Idx];
            }

            Sml_Assert(FreeBlock);

            sml_u32 Prev = this->PrevArray[Idx];
            sml_u32 Next = this->NextArray[Idx];

            Prev == this->Invalid ? this->Head = Next : this->PrevArray[Next] = Prev;
            Next == this->Invalid ? this->Tail = Prev : this->NextArray[Prev] = Next;

            sml_heap_block Result = {};
            Result.Data   = (sml_u8*)this->PushBase + FreeBlock->At;
            Result.Size   = RequestedSize;
            Result.At     = FreeBlock->At;
            Result.IntIdx = FreeBlock->IntIdx;

            size_t SizeDiff = FreeBlock->Size - RequestedSize;
            if(SizeDiff > 0)
            {
                sml_heap_block Extra = {};
                Extra.Data   = (sml_u8*)this->PushBase + FreeBlock->At + RequestedSize;
                Extra.Size   = SizeDiff;
                Extra.At     = FreeBlock->At + RequestedSize;
                Extra.IntIdx = this->IdxStack[--this->IdxStackCount];

                this->InsertFreeBlock(Extra);
            }

            return Result;
        }
        else
        {
            sml_heap_block Block = {};
            Block.Data = (sml_u8*)this->PushBase + this->PushSize;
            Block.Size = RequestedSize;
            Block.At   = this->PushSize;

            this->PushSize += RequestedSize;

            return Block;
        }
    }

    void Free(sml_heap_block Block)
    {
        Sml_Assert(this->IdxStackCount < this->FreeListSize);

        this->InsertFreeBlock(Block);
        this->IdxStack[this->IdxStackCount++] = Block.IntIdx;
    }

    sml_heap_block Reallocate(sml_heap_block OldBlock, sml_u32 Growth)
    {
        auto NewBlock = this->Allocate(OldBlock.Size * Growth);
        memcpy(NewBlock.Data, OldBlock.Data, OldBlock.Size);
        this->Free(OldBlock);

        return NewBlock;
    }

    void InsertFreeBlock(sml_heap_block Block)
    {
        sml_u32 InsertAt = this->Head;
        while(InsertAt != this->Invalid && this->FreeList[InsertAt].At < Block.At)
        {
            InsertAt = this->NextArray[InsertAt];
        }

        sml_u32 NextAt = InsertAt == this->Invalid ? this->Invalid : this->NextArray[InsertAt];

        if (NextAt != this->Invalid)
        {
            this->NextArray[Block.IntIdx] = this->NextArray[InsertAt];
            this->PrevArray[NextAt] = Block.IntIdx;
        }
        else
        {
            this->Tail = Block.IntIdx;
        }

        if (InsertAt != this->Invalid)
        {
            this->PrevArray[Block.IntIdx] = InsertAt;
            this->NextArray[InsertAt] = Block.IntIdx;
        }
        else
        {
            this->Head = Block.IntIdx;
        }

        if(Block.Size > this->BiggestFreeBlock) this->BiggestFreeBlock = Block.Size;
    }
};

static auto SmlMemory = sml_memory(Sml_Megabytes(50));
