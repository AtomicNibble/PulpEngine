

template<class T>
X_INLINE XLinkIntrusive<T>::~XLinkIntrusive()
{
    removeFromList();
}

template<class T>
X_INLINE XLinkIntrusive<T>::XLinkIntrusive()
{
    // Mark this node as the end of the list, with no link offset
    nextNode_ = (T*)((size_t)this + 1 - 0);
    prevLink_ = this;
}

template<class T>
X_INLINE XLinkIntrusive<T>::XLinkIntrusive(size_t offset)
{
    // Mark this node as the end of the list, with the link offset set
    nextNode_ = (T*)((size_t)this + 1 - offset);
    prevLink_ = this;
}

template<class T>
X_INLINE void XLinkIntrusive<T>::setOffset(size_t offset)
{
    // Mark this node as the end of the list, with the link offset set
    nextNode_ = (T*)((size_t)this + 1 - offset);
    prevLink_ = this;
}

template<class T>
X_INLINE XLinkIntrusive<T>* XLinkIntrusive<T>::nextLink()
{
    // Calculate the offset from a node pointer to a link structure
    size_t offset = (size_t)this - ((size_t)prevLink_->nextNode_ & ~1);

    // Get the link field for the next node
    return (XLinkIntrusive<T>*)(((size_t)nextNode_ & ~1) + offset);
}

template<class T>
X_INLINE void XLinkIntrusive<T>::removeFromList()
{
    nextLink()->prevLink_ = prevLink_;

    prevLink_->nextNode_ = nextNode_;
}

template<class T>
X_INLINE void XLinkIntrusive<T>::insertBefore(T* node, selfT* nextLink)
{
    removeFromList();

    prevLink_ = nextLink->prevLink_;
    nextNode_ = prevLink_->nextNode_;

    nextLink->prevLink_->nextNode_ = node;
    nextLink->prevLink_ = this;
}

template<class T>
X_INLINE void XLinkIntrusive<T>::insertAfter(T* node, selfT* prevLink)
{
    removeFromList();

    prevLink_ = prevLink;
    nextNode_ = prevLink->nextNode_;

    prevLink->nextLink()->prevLink_ = this;
    prevLink->nextNode_ = node;
}

template<class T>
X_INLINE bool XLinkIntrusive<T>::isLinked() const
{
    return prevLink_ != this;
}

template<class T>
X_INLINE void XLinkIntrusive<T>::unlink()
{
    removeFromList();

    // Mark this node as the end of the list with no link offset
    nextNode_ = (T*)((size_t)this + 1);
    prevLink_ = this;
}

template<class T>
X_INLINE XLinkIntrusive<T>* XLinkIntrusive<T>::prevLink()
{
    return prevLink_;
}

template<class T>
X_INLINE T* XLinkIntrusive<T>::prev()
{
    T* prevNode = prevLink_->prevLink_->nextNode_;
    if ((size_t)prevNode & 1)
        return nullptr;

    return prevNode;
}

template<class T>
X_INLINE const T* XLinkIntrusive<T>::prev() const
{
    const T* prevNode = prevLink_->prevLink_->nextNode_;

    if ((size_t)prevNode & 1)
        return nullptr;

    return prevNode;
}

template<class T>
X_INLINE T* XLinkIntrusive<T>::next()
{
    if ((size_t)nextNode_ & 1)
        return nullptr;

    return nextNode_;
}

template<class T>
X_INLINE const T* XLinkIntrusive<T>::next() const
{
    if ((size_t)nextNode_ & 1)
        return nullptr;

    return nextNode_;
}

// ----------------------------------------------------------------

template<class T>
XListIntrusive<T>::XListIntrusive() :
    link_(),
    offset_((size_t)-1)
{
}

template<class T>
XListIntrusive<T>::XListIntrusive(size_t offset) :
    link_(offset),
    offset_(offset)
{
}

template<class T>
XListIntrusive<T>::~XListIntrusive()
{
    unlinkAll();
}

template<class T>
bool XListIntrusive<T>::isEmpty() const
{
    return link_.next() == nullptr;
}

template<class T>
void XListIntrusive<T>::unlinkAll()
{
    for (;;) {
        XLinkIntrusive<T>* link = link_.prevLink();
        if (link == &link_) {
            break;
        }
        link->unlink();
    }
}

template<class T>
void XListIntrusive<T>::deleteAll(core::MemoryArenaBase* arena)
{
    X_ASSERT_NOT_NULL(arena);
    while (T* node = link_.next()) {
        X_DELETE(node, arena);
    }
}

template<class T>
T* XListIntrusive<T>::head()
{
    return link_.next();
}

template<class T>
T* XListIntrusive<T>::tail()
{
    return link_.prev();
}

template<class T>
const T* XListIntrusive<T>::head() const
{
    return link_.next();
}

template<class T>
const T* XListIntrusive<T>::tail() const
{
    return link_.prev();
}

template<class T>
T* XListIntrusive<T>::prev(T* node)
{
    return getLinkFromNode(node)->prev();
}

template<class T>
const T* XListIntrusive<T>::prev(const T* node) const
{
    return getLinkFromNode(node)->prev();
}

template<class T>
T* XListIntrusive<T>::next(T* node)
{
    return getLinkFromNode(node)->next();
}

template<class T>
const T* XListIntrusive<T>::next(const T* node) const
{
    return getLinkFromNode(node)->next();
}

template<class T>
void XListIntrusive<T>::insertHead(T* node)
{
    insertAfter(node, nullptr);
}

template<class T>
void XListIntrusive<T>::insertTail(T* node)
{
    insertBefore(node, nullptr);
}

template<class T>
void XListIntrusive<T>::insertBefore(T* node, T* before)
{
    X_ASSERT(!((size_t)node & 1), "pointer is not 2 byte aligned")
    (node);

    getLinkFromNode(node)->insertBefore(
        node,
        before ? getLinkFromNode(before) : &link_);
}

template<class T>
void XListIntrusive<T>::insertAfter(T* node, T* after)
{
    X_ASSERT(!((size_t)node & 1), "pointer is not 2 byte aligned")
    (node);

    getLinkFromNode(node)->insertAfter(
        node,
        after ? getLinkFromNode(after) : &link_);
}

template<class T>
XLinkIntrusive<T>* XListIntrusive<T>::getLinkFromNode(const T* node) const
{
    X_ASSERT(offset_ != (size_t)-1, "offset is not valid")
    (offset_);

    return (XLinkIntrusive<T>*)((size_t)node + offset_);
}