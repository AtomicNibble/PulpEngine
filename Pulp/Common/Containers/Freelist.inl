
inline Freelist::Freelist(void) :
    next_(nullptr)
{
}

inline Freelist::Freelist(void* start, void* end, size_t originalElementSize, size_t alignment, size_t offset) :
    next_(nullptr)
{
    size_t minimumSize = Max<size_t>(sizeof(next_), originalElementSize);
    size_t elementSize = bitUtil::RoundUpToMultiple(minimumSize, alignment);

    char* pAligned = pointerUtil::AlignTop<char>((char*)start + offset, alignment) - offset;

    union
    {
        void* as_void;
        char* as_char;
        Freelist* as_self;
    };

    as_void = pAligned;
    next_ = as_self;

    if (end <= as_void) {
        X_ASSERT(false, "Memory provided for free lists dose not satisfy even one element")(originalElementSize, elementSize, alignment, offset);
    }

    // initialize the free list - make every m_next of each element point to the next element in the list
    size_t numElements = ((char*)end - as_char) / elementSize;

    // next_ points to start of memory given.
    Freelist* runner = next_;

    as_char += elementSize;

    // we must now point all the objects to each other.
    for (size_t i = 1; i < numElements; ++i) {
        runner->next_ = as_self;
        runner = as_self;
        as_char += elementSize;
    }

    runner->next_ = nullptr;
}

inline void* Freelist::Obtain(void)
{
    // is there an entry left?
    if (next_ == nullptr) {
        return nullptr; // we are out of entries
    }

    // obtain one element from the head of the free list
    Freelist* head = next_;
    next_ = head->next_;
    return head;
}

inline void Freelist::Return(void* ptr)
{
    X_ASSERT_NOT_NULL(ptr);

    // put the returned element at the head of the free list
    Freelist* head = static_cast<Freelist*>(ptr);
    head->next_ = next_;
    next_ = head;
}
