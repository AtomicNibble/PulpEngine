#pragma once

#ifndef X_LINKED_LIST_H_
#define X_LINKED_LIST_H_

X_NAMESPACE_BEGIN(core)

//
//	this is intrusive also.
//	works a bit diffrent tho.
//
//
//	struct MyData
//	{
//		int value;
//		XLinkedList<MyData> node;
//	};
//
//
//	XLinkedList<MyData> nodes;
//
//
//	void foo()
//	{
//		MyData data;
//
//		data.node.addToEnd(nodes);
//
//	}
template<typename T>
class XLinkedList // 32 bytes in size.
{
public:
    typedef T type;

public:
    XLinkedList();
    ~XLinkedList();

    bool isListEmpty(void) const;
    bool inList(void) const;
    // O(n) - (not cache friendly tho. aka a list ^^)
    int num(void) const;
    void clear(void);

    void insertBefore(XLinkedList& node);
    void insertAfter(XLinkedList& node);
    void addToEnd(XLinkedList& node);
    void addToFront(XLinkedList& node);

    void remove(void);

    type* next(void) const;
    type* prev(void) const;

    type* owner(void) const;
    void setOwner(type* pObject);

    XLinkedList* listHead(void) const;
    XLinkedList* nextNode(void) const;
    XLinkedList* prevNode(void) const;

private:
    XLinkedList* pHead_;
    XLinkedList* pNext_;
    XLinkedList* pPrev_;

    type* pOwner_;
};

#include "LinkedList.inl"

X_NAMESPACE_END

#endif // !X_LINKED_LIST_H_