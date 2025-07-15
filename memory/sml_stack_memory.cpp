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
    sml_u32         NextIdx;

    sml_u32 *NextArray;
    sml_u32 *PrevArray;
    sml_u32  Head;
    sml_u32  Tail;

    // Meta-data
    bool ResizeOnFull;

    static constexpr sml_u32 Invalid      = sml_u32(-1);
    static constexpr sml_u32 FreeListSize = 1000;

    sml_memory(){};
    sml_memory(size_t HeapSize, bool ResizeOnFull = false)
    {
        this->PushBase     = malloc(HeapSize);
        this->PushSize     = 0;
        this->PushCapacity = HeapSize;

        this->FreeCount = 0;
        this->FreeList  =
            (sml_heap_block*)malloc(this->FreeListSize * sizeof(sml_heap_block));

        this->NextArray = (sml_u32*)malloc(this->FreeListSize * sizeof(sml_u32) * 2);
        this->PrevArray = this->NextArray + this->FreeListSize;
        this->Head      = this->Invalid;
        this->Tail      = this->Invalid;

        this->ResizeOnFull = ResizeOnFull;
        this->NextIdx = 0;

        memset(this->PushBase , 0, HeapSize);
        memset(this->FreeList , 0, this->FreeListSize * sizeof(sml_heap_block));

        for (sml_u32 Idx = 0; Idx < this->FreeListSize; ++Idx) 
        {
            this->NextArray[Idx] = this->PrevArray[Idx] = this->Invalid;
        }
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
                Sml_Assert(!"Resizing not implemented.");
            }
            else
            {
                Sml_Assert(!"Out of memory.");
            }
        }

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

        if (FreeBlock)
        {
            sml_u32 Prev = this->PrevArray[Idx];
            sml_u32 Next = this->NextArray[Idx];

            if (Prev == this->Invalid)
            {
                this->Head = Next;
                if (this->Head != this->Invalid)
                {
                    this->PrevArray[this->Head] = this->Invalid;
                }
                else
                {
                    this->Tail = this->Invalid;
                }
            }
            else if (Next == this->Invalid)
            {
                this->Tail = Prev;
                if (this->Tail != this->Invalid)
                {
                    this->NextArray[this->Tail] = this->Invalid;
                }
                else
                {
                    this->Head = this->Invalid;
                }
            }
            else
            {
                this->NextArray[Prev] = Next;
                this->PrevArray[Next] = Prev;

                this->NextArray[Idx] = this->Invalid;
                this->PrevArray[Idx] = this->Invalid;
            }

            sml_heap_block Result = {};
            Result.Data   = (sml_u8*)this->PushBase + FreeBlock->At;
            Result.Size   = RequestedSize;
            Result.At     = FreeBlock->At;
            Result.IntIdx = FreeBlock->IntIdx;

            size_t SizeDiff = FreeBlock->Size - RequestedSize;
            if (SizeDiff > 0)
            {
                sml_heap_block Extra = {};
                Extra.Data   = (sml_u8*)this->PushBase + FreeBlock->At + RequestedSize;
                Extra.Size   = SizeDiff;
                Extra.At     = FreeBlock->At + RequestedSize;
                Extra.IntIdx = this->NextIdx++;

                this->Free(Extra);
            }

            --this->FreeCount;

            return Result;
        }    
        else
        {
            sml_heap_block Block = {};
            Block.Data   = (sml_u8*)this->PushBase + this->PushSize;
            Block.Size   = RequestedSize;
            Block.At     = this->PushSize;
            Block.IntIdx = this->NextIdx++;

            this->PushSize += RequestedSize;

            return Block;
        }
    }

    void Free(sml_heap_block Block)
    {
        Sml_Assert(Block.IntIdx < this->FreeListSize);

        this->FreeList[Block.IntIdx] = Block;

        if (this->Head == this->Invalid)
        {
            this->Head = Block.IntIdx;
            this->Tail = Block.IntIdx;
        }
        else
        {
            sml_u32 InsertAt = this->Head;
            while (InsertAt != this->Invalid && Block.At > this->FreeList[InsertAt].At)
            {
                InsertAt = this->NextArray[InsertAt];
            }

            if (InsertAt == this->Head)
            {
                Sml_Assert(this->FreeList[InsertAt].At > Block.At);

                this->NextArray[Block.IntIdx] = this->Head;
                this->PrevArray[Block.IntIdx] = this->Invalid;

                this->PrevArray[this->Head] = Block.IntIdx;

                this->Head = Block.IntIdx;
            }
            else if (InsertAt == this->Invalid)
            {
                this->NextArray[Block.IntIdx] = this->Invalid;
                this->PrevArray[Block.IntIdx] = this->Tail;

                this->NextArray[this->Tail] = Block.IntIdx;

                this->Tail = Block.IntIdx;
            }
            else
            {
                this->NextArray[Block.IntIdx] = InsertAt;
                this->PrevArray[Block.IntIdx] = this->PrevArray[InsertAt];

                this->PrevArray[InsertAt] = Block.IntIdx;

                this->NextArray[this->PrevArray[Block.IntIdx]] = Block.IntIdx;
            }
        }

        ++this->FreeCount;
    }

    sml_heap_block Reallocate(sml_heap_block OldBlock, sml_u32 Growth)
    {
        auto NewBlock = this->Allocate(OldBlock.Size * Growth);
        memcpy(NewBlock.Data, OldBlock.Data, OldBlock.Size);

        this->Free(OldBlock);

        return NewBlock;
    }
};

static auto SmlMemory = sml_memory(Sml_Megabytes(50));