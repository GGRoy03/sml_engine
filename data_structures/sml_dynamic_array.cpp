#include <type_traits> // static type checking

template <typename T>
struct sml_dynamic_array
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "sml_dynamic_array<T> requires T to be trivially copyable");

    T*             Values;
    sml_u32        Count;
    sml_u32        Capacity;
    sml_heap_block Heap;

    sml_dynamic_array(){};
    sml_dynamic_array(sml_u32 InitialCount, bool ZeroInit = true)
    {
        if(InitialCount == 0) InitialCount = 8;

        this->Heap     = SmlMemory.Allocate(InitialCount * sizeof(T));
        this->Values   = (T*)this->Heap.Data;
        this->Count    = 0;
        this->Capacity = InitialCount;

        if(ZeroInit)
        {
            memset(this->Heap.Data, 0, this->Capacity * sizeof(T));
        }
    }

    inline void Push(T Value)
    {
        Sml_Assert(this->Values);

        if(this->Count == this->Capacity)
        {
            this->Capacity *= 2;
            this->Heap      = SmlMemory.Reallocate(this->Heap, 2);
            this->Values    = (T*)this->Heap.Data;
        }

        this->Values[Count++] = Value;
    }

    inline void Free()
    {
        Sml_Assert(this->Values);

        SmlMemory.Free(this->Heap);

        memset(this, 0, sizeof(this));
    }

    inline void Reset()
    {
        this->Count = 0;
    }

    T& operator[](sml_u32 Index)
    {
        Sml_Assert(Index < this->Capacity);

        return this->Values[Index];
    }

    const T& operator[](sml_u32 Index) const
    {
        Sml_Assert(Index < this->Count);
        return this->Values[Index];
    }

    T* begin() { return this->Values; }
    T* end()   { return this->Values + this->Count; }
};
