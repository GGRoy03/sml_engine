#include <type_traits> // static type checking
#include <limits>      // Invalid Idx for type (No warnings)

template <typename D, typename F>
struct slot_map
{
    // Core-data
    D      *Data;
    sml_u32 Capacity;

    // Free-list
    F      *FreeList;
    sml_u32 FreeCount;

    // Active-list
    F *Next;
    F *Prev;
    F  Head;
    F  Tail;

    // Heap
    sml_heap_block DataHeap;
    sml_heap_block FreeListHeap;
    sml_heap_block ActiveListHeap;

    // Meta
    bool ResizeOnFull = false;

    static constexpr F Invalid = std::numeric_limits<F>::max();

    slot_map(){};
    slot_map(sml_u32 InitialSize, bool ResizeOnFull = false, bool ZeroInit = true)
    {
        if(InitialSize == 0) InitialSize = 8;

        this->Capacity = InitialSize;

        this->DataHeap = SmlMemory.Allocate(this->Capacity * sizeof(D));
        this->Data     = (D*)this->DataHeap.Data;

        this->FreeListHeap = SmlMemory.Allocate(this->Capacity * sizeof(F));
        this->FreeList     = (F*)this->FreeListHeap.Data;
        this->FreeCount    = this->Capacity;

        this->ActiveListHeap = SmlMemory.Allocate(this->Capacity * sizeof(F) * 2);
        this->Next           = (F*)this->ActiveListHeap.Data;
        this->Prev           = (F*)this->Next + this->Capacity;

        this->Head = this->Invalid;
        this->Tail = this->Invalid;

        for(sml_u32 Idx = 0; Idx < this->Capacity; Idx++)
        {
            this->FreeList[Idx] = F(this->Capacity - 1 - Idx);
        }

        this->ResizeOnFull = ResizeOnFull;

        if(ZeroInit)
        {
            memset(this->Data               , 0, this->Capacity * sizeof(D));
            memset(this->ActiveListHeap.Data, 0, this->Capacity * sizeof(F) * 2);
        }
    }

    inline F Emplace(D NewData)
    {
        if(this->FreeCount > 0)
        {
            F Idx = this->FreeList[--this->FreeCount];
            this->Data[Idx] = NewData;

            this->Prev[Idx] = this->Tail;
            this->Next[Idx] = this->Invalid;
            if(this->Tail != Invalid) this->Next[Tail] = Idx;
            this->Tail = Idx;
            if(this->Head == Invalid) this->Head = Idx;

            return Idx;
        }
        else if(this->ResizeOnFull)
        {
            this->Capacity *= 2;

            this->DataHeap     = SmlMemory.Reallocate(this->DataHeap    , 2);
            this->FreeListHeap = SmlMemory.Reallocate(this->FreeListHeap, 2);

            Sml_Assert(!"Not implemented");

            // BUG: Placeholder
            return 0;
        }
        else
        {
            Sml_Assert(!"Slot map is full.");
            return 0;
        }
    }

    inline void Remove(F Idx)
    {
        if(this->FreeCount < this->Capacity)
        {
            this->FreeList[this->FreeCount++] = Idx;

            F PrevSlot = this->Prev[Idx];
            F NextSlot = this->Next[Idx];

            PrevSlot == this->Invalid ? this->Head = NextSlot :
                                        this->Prev[NextSlot] = PrevSlot;

            NextSlot == this->Invalid ? this->Tail = PrevSlot :
                                        this->Next[PrevSlot] = NextSlot;

            memset(this->Data + Idx, 0, sizeof(D));
        }
        else
        {
            Sml_Assert(!"Free list already full.");
        }
    }

    inline F GetFreeSlot()
    {
        if(this->FreeCount > 0)
        {
            F Slot = this->FreeList[--this->FreeCount];
            return Slot;
        }
        else
        {
            Sml_Assert(!"No free slots remaining. Resize?");
            return this->Invalid;
        }
    }

    D& operator[](F Idx) noexcept
    { 
        return this->Data[Idx]; 
    }
};
