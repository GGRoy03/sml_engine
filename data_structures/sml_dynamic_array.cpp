#include <type_traits> // static type checking

template <typename T>
struct sml_dynamic_array
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "sml_dynamic_array<T> requires T to be trivially copyable");

    T*               Values;
    sml_u32          Count;
    sml_u32          Capacity;
    sml_memory_block Heap;

    sml_dynamic_array(){};
    sml_dynamic_array(sml_u32 InitialCount, bool ZeroInit = true)
    {
        if(InitialCount == 0) InitialCount = 8;

        this->Heap     = SmlInt_PushMemory(InitialCount * sizeof(T));
        this->Values   = (T*)this->Heap.Data;
        this->Count    = 0;
        this->Capacity = InitialCount;

        if(ZeroInit)
        {
            memset(this->Heap.Data, 0, this->Capacity * sizeof(T));
        }
    }

    void Push(T Value)
    {
        Sml_Assert(this->Values);

        if(this->Count == this->Capacity)
        {
            this->Capacity *= 2;
            this->Heap      = SmlInt_ReallocateMemory(this->Heap, 2);
        }

        this->Values[Count++] = Value;
    }

    T* PushNext()
    {
        Sml_Assert(this->Values);

        if(this->Count == this->Capacity)
        {
            this->Capacity *= 2;
            this->Heap     = SmlInt_ReallocateMemory(&this->Heap, 2);
        }

        T *Ptr = this->Values + this->Count;
        this->Count++;

        return Ptr;
    }

    T Pop()
    {
        Sml_Assert(this->Values);
        Sml_Assert(this->Count > 0);

        T Value      = this->Values[this->Count - 1];
        this->Count -= 1;

        return Value;
    }

    void Free()
    {
        Sml_Assert(this->Values);

        SmlInt_FreeMemory(this->Heap);

        this->Values   = nullptr;
        this->Count    = 0;
        this->Capacity = 0;
    }

    void Reset()
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
};
