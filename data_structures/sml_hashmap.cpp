template<typename K, typename V>
struct sml_hashmap_entry
{
    K Key;
    V Value;
};

template<typename K, typename V>
struct sml_hashmap
{
    sml_u8                  *MetaData;
    sml_hashmap_entry<K, V> *Buckets;
    sml_u32                  GroupCount;

    sml_memory_block BucketHeap;
    sml_memory_block MetaDataHeap;

    const sml_u32 BucketGroupSize = 16;
    const sml_u8  EmptyBucketTag  = 0x80;

    sml_hashmap<K, V>(){};
    sml_hashmap<K, V>(sml_u32 InitialCount)
    {
        if(InitialCount == 0) InitialCount = 8;

        this->GroupCount = InitialCount;

        this->GroupCount = InitialCount;
        if ((this->GroupCount & (this->GroupCount - 1)) != 0)
        {
            sml_u32 Pow2 = 1;
            while (Pow2 < this->GroupCount) Pow2 <<= 1;
            this->GroupCount = Pow2;
        }

        sml_u32 BucketCount          = this->GroupCount * this->BucketGroupSize;
        size_t  BucketAllocationSize = BucketCount * sizeof(sml_hashmap_entry<K, V>);

        this->BucketHeap = SmlInt_PushMemory(BucketAllocationSize);
        this->Buckets    = (sml_hashmap_entry<K, V>*)this->BucketHeap.Data;

        this->MetaDataHeap = SmlInt_PushMemory(BucketCount * sizeof(sml_u8));
        this->MetaData     = (sml_u8*)this->MetaDataHeap.Data;

        memset(this->MetaData, this->EmptyBucketTag, BucketCount * sizeof(sml_u8));
    }

    void Insert(K Key, V Value)
    {
        sml_u32 ProbeCount  = 0;
        sml_u64 HashedValue = XXH64(&Key, sizeof(Key), 0);
        sml_u32 GroupIndex  = HashedValue & (this->GroupCount - 1);

        while(true)
        {
            sml_u8 *Meta = this->MetaData + (GroupIndex * this->BucketGroupSize);
            sml_u8  Tag  = (HashedValue & 0x7F);

            Sml_Assert(Tag < this->EmptyBucketTag);

            __m128i MetaVector = _mm_loadu_si128((__m128i*)Meta);
            __m128i TagVector  = _mm_set1_epi8(Tag);

            sml_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

            while(Mask)
            {
                sml_i32 Lane  = (i32)ctz32(Mask);
                sml_u32 Index = (GroupIndex * this->BucketGroupSize) + Lane;

                sml_hashmap_entry<K, V> *Entry = this->Buckets + Index;
                if(Entry->Key == Key)
                {
                    return;
                }

                Mask &= Mask - 1;
            }

            __m128i EmptyVector = _mm_set1_epi8(this->EmptyBucketTag);
            sml_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector,
                                                                   EmptyVector));

            if(MaskEmpty)
            {
                sml_i32 Lane  = ctz32(MaskEmpty);
                sml_u32 Index = (GroupIndex * this->BucketGroupSize) + Lane;

                sml_hashmap_entry<K, V> *Entry = this->Buckets + Index;
                Entry->Key   = Key;
                Entry->Value = Value;

                Meta[Lane] = Tag;

                return;
            }

            ProbeCount++;
            GroupIndex = (GroupIndex+(ProbeCount*ProbeCount))&(this->GroupCount-1);
        }
    }
};
