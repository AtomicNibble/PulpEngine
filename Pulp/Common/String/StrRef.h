#pragma once

#ifndef _X_REFSTRING_H_
#define _X_REFSTRING_H_

// TODO: remove
#include <string.h>
#include <wchar.h>
#include <stdio.h>

#include "String\StringUtil.h"

X_NAMESPACE_BEGIN(core)

template<typename CharT>
class StringRef
{
public:
    // all the types
    typedef StringRef<CharT> StrT;
    // the size is actually stored as a 32bit uint. (save 8 bytes in header (2 size values))
    // but everything else is size_t, so i don't need to cast all other the place.
    // still allows for a 4gb string lol.
    typedef size_t size_type;
    typedef CharT value_type;
    typedef const value_type* const_str;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef pointer iterator;
    typedef const_pointer const_iterator;

    typedef int length_type;

public:
    StringRef();
    // from another string object
    StringRef(const StrT& str);
    // from another string object 'move'
    StringRef(StrT&& str);
    // from another string object, define the offset & count
    StringRef(const StrT& str, size_type offset, size_type count);
    // const from char (optionaly repeat it x number of times)
    explicit StringRef(value_type ch, size_type numRepeat = 1);
    // from a const str (allocates memory & copyies)
    explicit StringRef(const_str str);
    // const from string + length saves a length
    StringRef(const_str str, size_type length);
    // const from a begin(), end() saves a length cal
    StringRef(const_iterator first, const_iterator last);

    // removes a refrence from the string, delete if == 0
    ~StringRef();

    static const size_type npos = std::numeric_limits<size_type>::max();

public:
    constexpr const_pointer c_str(void) const;
    constexpr const_pointer data(void) const;

    // iterator
    constexpr const_iterator begin(void) const;
    constexpr const_iterator end(void) const;

    // the length of the string
    constexpr size_type length(void) const;
    // same as length
    constexpr size_type size(void) const;
    // returns current size of allocated memory
    constexpr size_type capacity(void) const;

    bool isEmpty(void) const;
    bool isNotEmpty(void) const;

    // clears the string and de-inc the ref
    void clear(void);

    // Sets the capacity of the string to a number at least as great as a specified number.
    void reserve(size_type size);
    // allocates and fills both size() & capacity() == size
    void resize(size_type size, value_type ch = ' ');
    // shrink memory allocated to equal the length of the string.
    void shrinkToFit(void);

    // char& operator[] (size_t pos); <- would have to make new strings if refred.
    const value_type& operator[] (size_t pos) const;

    // overloaded assignment
    StrT& operator=(const StrT& str);
    StrT& operator=(StrT&& str);
    StrT& operator=(value_type ch);
    StrT& operator=(const_str str);

    // string concatenation
    StrT& operator+=(const StrT& str);
    StrT& operator+=(value_type ch);
    StrT& operator+=(const_str str);

    // Case util
    StrT& toLower(void);
    StrT& toUpper(void);

    // Trim it good
    StrT& trim(void);
    StrT& trim(value_type ch);     // trim this char
    StrT& trim(const_str charSet); // trim this set of chars.

    StrT& trimLeft(void);
    StrT& trimLeft(value_type ch);
    StrT& trimLeft(const_str charSet);

    StrT& trimRight(void);
    StrT& trimRight(value_type ch);
    StrT& trimRight(const_str charSet);

    // append maybe mend?
    StrT& append(const value_type* _Ptr);
    StrT& append(const value_type* _Ptr, size_type count);
    StrT& append(const StrT& _Str, size_type nOff, size_type count);
    StrT& append(const StrT& _Str);
    StrT& append(size_type count, value_type _Ch);
    StrT& append(const_iterator _First, const_iterator _Last);

    // assign or to sign?
    StrT& assign(const_str _Ptr);
    StrT& assign(const_str _Ptr, size_type count);
    StrT& assign(const StrT& _Str, size_type off, size_type count);
    StrT& assign(const StrT& _Str);
    StrT& assign(size_type count, value_type _Ch);
    StrT& assign(const_iterator _First, const_iterator _Last);

    // replace part of string.
    StrT& replace(value_type chOld, value_type chNew);
    StrT& replace(const_str strOld, const_str strNew);
    StrT& replace(size_type pos, size_type count, const_str strNew);                   // replace at offset the number of chars in count with the contents of new
    StrT& replace(size_type pos, size_type count, const_str strNew, size_type count2); // same as above but only use count2 from new.
    StrT& replace(size_type pos, size_type count, size_type nNumChars, value_type chNew);

    // insert new elements to string.
    StrT& insert(size_type nIndex, value_type ch);
    StrT& insert(size_type nIndex, size_type count, value_type ch);
    StrT& insert(size_type nIndex, const_str pstr);
    StrT& insert(size_type nIndex, const_str pstr, size_type count);

    //! delete count characters starting at zero-based index
    StrT& erase(size_type nIndex, size_type count = npos);

    // compare the goat.com
    bool compare(const StrT& Str) const;
    bool compare(const_str ptr) const;
    bool compare(const_str ptr, size_type length) const;

    //	<0	the first character that does not match has a lower
    //		value in ptr1 than in ptr2
    //	0	the contents of both strings are equal
    //	>0	the first character that does not match has a greater
    //		value in ptr1 than in ptr2
    int compareInt(const StrT& _Str) const;
    int compareInt(const_str ptr) const;

    // Case insensitive compare
    bool compareCaseInsen(const StrT& _Str) const;
    bool compareCaseInsen(const_str ptr) const;

    // find
    const_str find(value_type ch) const;
    const_str find(const_str subStr) const;

    // swap a pickle
    void swap(StrT& oth);

    //! simple sub-string extraction
    //	StrT substr(size_type pos, const_str end = nullptr) const;
    StrT substr(const_str pBegin, const_str pEnd = nullptr) const;

    // to the left..
    StrT left(size_type count) const;
    StrT right(size_type count) const;

private:
    // used internaly
    static void _copy(value_type* dest, const value_type* src, size_type srcLen);
    static void _move(value_type* dest, const value_type* src, size_type srcLen);
    static void _set(value_type* dest, value_type ch, size_type numRepeat);

protected:
    X_PRAGMA(pack(push, 4))
    struct XStrHeader
    {
        XStrHeader() :
            refCount(0),
            length(0),
            allocSize(0){};
        XStrHeader(int ref, length_type length, length_type alloc) :
            refCount(ref),
            length(length),
            allocSize(alloc){};

        void addRef(void)
        {
            refCount++;
        };
        int release(void)
        {
            return --refCount;
        };
        pointer getChars(void)
        {
            return (value_type*)(this + 1);
        }

        int refCount;
        length_type length;
        length_type allocSize;
    };
    X_PRAGMA(pack(pop))

    static XStrHeader* emptyHeader(void)
    {
        X_DISABLE_WARNING(4640)
        static XStrHeader EmptyString[2] = {{-1, 0, 0}, {0, 0, 0}};
        return &EmptyString[0];
        X_ENABLE_WARNING(4640)
    }

    XStrHeader* header(void) const;

    // dose not check current length
    void Allocate(size_type length);
    void free(void);
    void SetEmpty(void);
    void makeUnique(void);

    void Concatenate(const_str sStr1, size_type nLen1, const_str sStr2, size_type nLen2);
    void ConcatenateInPlace(const_str sStr, size_type length);
    void _Assign(const_str sStr, size_type nLen);

    static void freeData(XStrHeader* pData);
    static size_t strlen(const_str pStr);

protected:
    // i might store the string length here.
    // makes the object twice as big, which may be problematic for
    // keeping structures inside cace lanes sizes.
    // advantages are reduced str length tests.
    // i'll leave it for now, see if it's a issue when profiling.

    value_type* str_; // pointer to the string data.
};


typedef StringRef<char> string;

#include "StrRef.inl"

X_NAMESPACE_END

#endif // _X_REFSTRING_H_