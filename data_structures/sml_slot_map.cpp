#include <type_traits> // static type checking

template <typename D, typename F>
struct sml_slot_map
{
    sml_u32 Capacity;

    D* Data;

    F*      FreeList;
    sml_u32 FreeCount;

    sml_memory_block DataHeap;
    sml_memory_block FreeListHeap;

    bool ResizeOnFull = false;

    sml_slot_map(){};
    sml_slot_map(sml_u32 InitialSize, bool ResizeOnFull = false, bool ZeroInit = true)
    {
        if(InitialSize == 0) InitialSize = 8;

        this->Capacity = InitialSize;

        this->DataHeap = SmlInt_PushMemory(this->Capacity * sizeof(D));
        this->Data     = (D*)this->DataHeap.Data;

        this->FreeListHeap = SmlInt_PushMemory(this->Capacity * sizeof(F));
        this->FreeList     = (F*)this->FreeListHeap.Data;
        this->FreeCount    = this->Capacity;

        for(sml_u32 Idx = 0; Idx < this->Capacity; Idx++)
        {
            this->FreeList[Idx] = F(this->Capacity - 1 - Idx);
        }

        this->ResizeOnFull = ResizeOnFull;

        if(ZeroInit)
        {
            memset(this->Data, 0, this->Capacity * sizeof(D));
        }
    }

    F Emplace(D Data)
    {
        if(this->FreeCount > 0)
        {
            F Idx = this->FreeList[--this->FreeCount];

            Data[Idx] = Data;

            return Idx;
        }
        else if(this->ResizeOnFull)
        {
            this->Capacity *= 2;

            this->DataHeap     = SmlInt_ReallocateMemory(this->DataHeap    , 2);
            this->FreeListHeap = SmlInt_ReallocateMemory(this->FreeListHeap, 2);
        }
        else
        {
            Sml_Assert(!"Slot map is full.");
            return 0;
        }
    }

    void Remove(F Idx)
    {
        if(this->FreeCount < this->Capacity)
        {
            this->FreeList[this->FreeCount++] = Idx;
            memset(this->Data + Idx, 0, sizeof(D));
        }
        else
        {
            Sml_Assert(!"Free list already full.");
        }
    }

    D& operator[](F Idx)
    { 
        return this->Data[Idx]; 
    }
};
