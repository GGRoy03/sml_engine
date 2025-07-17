template <typename T>
struct stack
{
    // Core
    T*      Values;
    sml_u32 Count;
    sml_u32 Capacity;

    // Misc
    sml_heap_block Heap;

    // Meta
    bool ResizeOnFull;

    stack(){};
    stack(sml_u32 InitialSize, bool ResizeOnFull = false, bool ZeroInit = true)
    {
        this->Heap     = SmlMemory.Allocate(InitialSize * sizeof(T));
        this->Values   = (T*)this->Heap.Data;
        this->Count    = 0;
        this->Capacity = InitialSize;

        this->ResizeOnFull = ResizeOnFull;

        if(ZeroInit)
        {
            memset(this->Values, 0, this->Heap.Size);
        }
    }

    inline void Push(T& Value)
    {
        Sml_Assert(this->Values);

        if(this->Count == this->Capacity)
        {
            if(this->ResizeOnFull)
            {
                this->Capacity *= 2;
                this->Heap      = SmlMemory.Reallocate(this->Heap, 2);
                this->Values    = (T*)this->Heap.Data;
            }
            else
            {
                Sml_Assert(!"Stack is specified as non-resizable and is full.");
                return;
            }
        }

        this->Values[this->Count++] = Value;
    }

    inline T& Peek()
    {
        Sml_Assert(this->Values && this->Count > 0);

        T& Value = this->Values[this->Count - 1];

        return Value;
    }

    inline T& Pop()
    {
        Sml_Assert(this->Values && this->Count > 0);

        T& Value = this->Values[--this->Count];

        return Value;
    }

    inline T& Under()
    {
        Sml_Assert(this->Values && this->Count > 0);

        T& Value = this->Values[0];

        return Value;
    }

    inline void Free()
    {
        SmlMemory.Free(this->Heap);

        memset(this, 0, sizeof(this));
    }

    inline bool Empty()
    {
        return this->Count == 0;
    }

    inline void Reset()
    {
        this->Count = 0;
    }
};
