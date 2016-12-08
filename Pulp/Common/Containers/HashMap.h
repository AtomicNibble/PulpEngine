#pragma once


#ifndef _X_CON_HASHMAP_H_
#define _X_CON_HASHMAP_H_


#include "HashBase.h"

#include <Hashing\Fnva1Hash.h>

#include <String\StrRef.h>

X_NAMESPACE_BEGIN(core)


inline size_t _Hash_seq(const unsigned char *_First, size_t _Count)
{
	return (size_t)Hash::Fnv1aHash(_First, _Count);
}


template<class _Ktype>
struct _Bitwise_hash
{	
	size_t operator()(const _Ktype& _Keyval) const {
		return (_Hash_seq((const unsigned char *)&_Keyval, sizeof (_Ktype)));
	}
};



template<class _Ktype>
struct hash : public _Bitwise_hash<_Ktype>
{	
	static const bool _Value = __is_enum(_Ktype);
};

template<>
struct hash<core::string>
{
	size_t operator()(const core::string& __s) const  
	{
		// we can't use the length as it might be a stack string.
		// this is not as bad as it seams.
		// since most hash finds we be from const char*
		// and if we did not do this, we would have to do a string len AND
		// memory allocation + copy the string.
		// so it's a win really.
		// for the cost of a few redundant strlen's
		size_t len = core::strUtil::strlen(__s);

		return (size_t)core::Hash::Fnv1aHash(__s.data(), len);
	}
};

template<size_t N>
struct hash<core::StackString<N>>
{
	size_t operator()(const core::StackString<N>& __s) const
	{
		return (size_t)core::Hash::Fnv1aHash(__s.c_str(), __s.length());
	}
};

template<typename CharT>
struct hash<core::Path<CharT>>
{
	size_t operator()(const core::Path<CharT>& __s) const
	{
		return (size_t)core::Hash::Fnv1aHash(__s.c_str(), __s.length());
	}
};


template<>
struct hash<const char*>
{
	size_t operator()(const char* const __s) const  {
		return (size_t)core::Hash::Fnv1aHash(__s, strlen(__s));
	}
};


template<>
struct hash<const char* const>
{
	size_t operator()(const char* const __s) const  {
		return (size_t)core::Hash::Fnv1aHash(__s, strlen(__s));
	}
};

template<class _Type = void>
struct equal_to
{	
	bool operator()(const _Type& _Left, const _Type& _Right) const {	
		return (_Left == _Right);
	}
};


template<>
struct equal_to<core::string>
{
	bool operator()(const core::string& _Left, const core::string& _Right) const {
		return (_Left == _Right);
	}
};

template<size_t N>
struct equal_to<core::StackString<N>>
{
	bool operator()(const core::StackString<N>& _Left, const core::StackString<N>& _Right) const {
		return (_Left == _Right);
	}
};

template<typename CharT>
struct equal_to<core::Path<CharT>>
{
	bool operator()(const core::Path<CharT>& _Left, const core::Path<CharT>& _Right) const {
		return (_Left == _Right);
	}
};


template<>
struct equal_to<const char*>
{
	bool operator()(const char* const _Left, const char* const _Right) const {
		return core::strUtil::IsEqual(_Left, _Right);
	}
};

template<>
struct equal_to<const char* const>
{
	bool operator()(const char* const _Left, const char* const _Right) const {
		return core::strUtil::IsEqual(_Left, _Right);
	}
};

template <class Key,
	class Value,
	class HashFn = hash<Key>,
	class EqualKey = equal_to<Key> >
class HashMap : public HashBase<Key, std::pair<const Key, Value>, HashFn, EqualKey>
{
public:
	typedef HashBase<Key, std::pair<const Key, Value>, HashFn, EqualKey> _base;

	typedef typename _base::key_type key_type;
	typedef Value data_type;
	typedef Value mapped_type;
	typedef typename _base::value_type value_type;
	typedef typename _base::hasher hasher;
	typedef typename _base::key_equal key_equal;


	typedef typename _base::size_type size_type;
	typedef typename _base::reference reference;
	typedef typename _base::const_reference const_reference;
	typedef typename _base::pointer pointer;
	typedef typename _base::const_pointer const_pointer;

	typedef typename _base::iterator			iterator;
	typedef typename _base::const_iterator		const_iterator;

	/// A constant defining the size of a single entry when stored in the hash map.
	static const size_t PER_ENTRY_SIZE = _base::PER_ENTRY_SIZE;
	


private:
//	typedef HashMap_node<value_type> _Node;

public:
	HashMap(MemoryArenaBase* arena) : HashBase(arena) {}
	explicit HashMap(MemoryArenaBase* arena, size_type num) : HashBase(arena, num) {}

	std::pair<iterator, bool> insert(const value_type& obj )
	{
		ensureSize(numElements_ + 1);
		return insertUniqueNoResize(obj);
	}

	// reserver a number of elements
	void reserve(size_type num)
	{
		ensureSize(num + 1);
	}


	data_type& operator[](const key_type& key) {
		return find(key)->second;
	}

	bool contains(const key_type& key) {
		return find(key) != end();
	}

	template <class MemoryArenaT>
	static inline size_t GetMemoryRequirement(size_t capacity)
	{
	//	size_type size = next_size(capacity);

		return (MemoryArenaT::getMemoryRequirement(sizeof(_Node)*capacity) +
			MemoryArenaT::getMemoryRequirement(sizeof(_Node*)*capacity));
	}

public:


};


X_NAMESPACE_END


#endif // !_X_CON_HASHMAP_H_
