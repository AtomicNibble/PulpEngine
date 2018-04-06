
X_DISABLE_WARNING(4355)

template<typename T>
X_INLINE XLinkedList<T>::XLinkedList() :
    pHead_(this),
    pNext_(this),
    pPrev_(this),
    pOwner_(nullptr)
{
}

X_ENABLE_WARNING(4355)

template<typename T>
X_INLINE XLinkedList<T>::~XLinkedList()
{
    clear();
}

template<typename T>
X_INLINE bool XLinkedList<T>::isListEmpty(void) const
{
    return pHead_->pNext_ == pHead_;
}

template<typename T>
X_INLINE bool XLinkedList<T>::inList(void) const
{
    return pHead_ != this;
}

template<typename T>
X_INLINE int XLinkedList<T>::num(void) const
{
    XLinkedList<type>* node;
    int num;

    num = 0;
    for (node = pHead_->pNext_; node != pHead_; node = node->pNext_) {
        num++;
    }

    return num;
}

template<typename T>
X_INLINE void XLinkedList<T>::clear(void)
{
    if (pHead_ == this) {
        while (pNext_ != this) {
            pNext_->remove();
        }
    }
    else {
        remove();
    }
}

template<typename T>
X_INLINE void XLinkedList<T>::insertBefore(XLinkedList& node)
{
    remove();

    pNext_ = &node;
    pPrev_ = node.pPrev_;
    node.pPrev_ = this;
    pPrev_->pNext_ = this;
    pHead_ = node.pHead_;
}

template<typename T>
X_INLINE void XLinkedList<T>::insertAfter(XLinkedList& node)
{
    remove();

    pPrev_ = &node;
    pNext_ = node.pNext_;
    node.pNext_ = this;
    pNext_->pPrev_ = this;
    pHead_ = node.pHead_;
}

template<typename T>
X_INLINE void XLinkedList<T>::addToEnd(XLinkedList& node)
{
    insertBefore(*node.pHead_);
}

template<typename T>
X_INLINE void XLinkedList<T>::addToFront(XLinkedList& node)
{
    insertAfter(*node.pHead_);
}

template<typename T>
X_INLINE void XLinkedList<T>::remove(void)
{
    pPrev_->pNext_ = pNext_;
    pNext_->pPrev_ = pPrev_;

    pNext_ = this;
    pPrev_ = this;
    pHead_ = this;
}

template<typename T>
X_INLINE typename XLinkedList<T>::type* XLinkedList<T>::next(void) const
{
    if (!pNext_ || (pNext_ == pHead_))
        return nullptr;

    return pNext_->owner();
}

template<typename T>
X_INLINE typename XLinkedList<T>::type* XLinkedList<T>::prev(void) const
{
    if (!pPrev_ || (pPrev_ == pHead_))
        return nullptr;

    return pPrev_->owner();
}

template<typename T>
X_INLINE typename XLinkedList<T>::type* XLinkedList<T>::owner(void) const
{
    return pOwner_;
}

template<typename T>
X_INLINE void XLinkedList<T>::setOwner(type* pObject)
{
    pOwner_ = pObject;
}

template<typename T>
X_INLINE XLinkedList<T>* XLinkedList<T>::listHead(void) const
{
    return pHead_;
}

template<typename T>
X_INLINE XLinkedList<T>* XLinkedList<T>::nextNode(void) const
{
    return pNext_;
}

template<typename T>
X_INLINE XLinkedList<T>* XLinkedList<T>::prevNode(void) const
{
    return pPrev_;
}