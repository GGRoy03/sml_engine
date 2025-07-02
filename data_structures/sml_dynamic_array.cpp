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
    sml_dynamic_array(sml_u32 InitialCount)
    {
        if(InitialCount == 0) InitialCount = 8;

        this->Heap     = SmlInt_PushMemory(InitialCount * sizeof(T));
        this->Values   = (T*)this->Heap.Data;
        this->Count    = 0;
        this->Capacity = InitialCount;
    }

    sml_dynamic_array& operator=(const sml_dynamic_array&) = delete;

    void Push(T Value)
    {
        Sml_Assert(this->Values);

        if(this->Count == this->Capacity)
        {
            this->Capacity *= 2;

            sml_memory_block NewBlock = SmlInt_PushMemory(this->Capacity * sizeof(T));

            memcpy(NewBlock.Data, this->Values, this->Count * sizeof(T));
            SmlInt_FreeMemory(&this->Heap);

            this->Heap = NewBlock;
        }

        this->Values[Count++] = Value;
    }

    void Free()
    {
        SmlInt_FreeMemory(&this->Heap);

        this->Values   = nullptr;
        this->Count    = 0;
        this->Capacity = 0;
    }
};
