#pragma once

#ifndef X_LINKED_LIST_INTRUSIVE_H_
#define X_LINKED_LIST_INTRUSIVE_H_

#include <Util\PointerFlags.h>

X_NAMESPACE_BEGIN(core)

// def need to UT this goat.
// check the pointer flags are all working sexy + member offsets.

template<class T>
class XLinkIntrusive // 16 bytes overhead(x64)
{
public:
	typedef T type;
	typedef XLinkIntrusive<T> selfT;

	~XLinkIntrusive();
	XLinkIntrusive();
	XLinkIntrusive(size_t offset);

	bool isLinked() const;
	void unlink();

	type* prev();
	type* next();
	const type* prev() const;
	const type* next() const;

	void setOffset(size_t offset);
	selfT* nextLink();
	selfT* prevLink();

	void insertBefore(type* node, selfT* nextLink);
	void insertAfter(type* node, selfT* prevLink);

private:
	void removeFromList();

	X_NO_COPY(XLinkIntrusive);
	X_NO_ASSIGN(XLinkIntrusive);

private:
	type*	nextNode_; // pointer to the next >object<
	selfT*  prevLink_; // pointer to the previous >link field<
};



template<class T>
class XListIntrusive
{
public:
	typedef T type;

	XListIntrusive();
	XListIntrusive(core::MemoryArenaBase* arena);
	~XListIntrusive();

	void setArena(core::MemoryArenaBase* arena);

	bool isEmpty() const;
	void unlinkAll();
	void deleteAll();

	T* head();
	T* tail();
	const T* head() const;
	const T* tail() const;

	T* prev(T* node);
	T* next(T* node);
	const T* prev(const T* node) const;
	const T* next(const T* node) const;

	void insertHead(T* node);
	void insertTail(T* node);
	void insertBefore(T* node, T* before);
	void insertAfter(T* node, T* after);

private:
	XLinkIntrusive<T>    link_;
	size_t				 offset_;
	core::MemoryArenaBase* arena_;

	XListIntrusive(size_t offset);
	XLinkIntrusive<T>* getLinkFromNode(const T* node) const;

	template<class T, size_t offset> 
	friend class XListIntrusiveDeclare;

	// Hide copy-constructor and assignment operator
	X_NO_COPY(XListIntrusive);
	X_NO_ASSIGN(XListIntrusive);
};


#include "LinkedListIntrusive.inl"


#define INTRUSIVE_LIST_DECLARE(T, link) core::XListIntrusiveDeclare<T, X_OFFSETOF(T, link)>
#define INTRUSIVE_LIST_LINK(T) core::XLinkIntrusive<T>
#define INTRUSIVE_LIST_PTR(T) core::XListIntrusive<T> *

// goaty little wrapper so can use default constructor.
template<class T, size_t offset>
class XListIntrusiveDeclare : public XListIntrusive<T>
{
public:
	XListIntrusiveDeclare();
};

template<class T, size_t offset>
XListIntrusiveDeclare<T, offset>::XListIntrusiveDeclare() :
	XListIntrusive<T>(offset)
{}


X_NAMESPACE_END

#endif // !X_LINKED_LIST_INTRUSIVE_H_