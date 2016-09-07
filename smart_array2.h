#ifndef _SMART2_ARRAY_H_INCLUDED_2014_04_10
#define _SMART2_ARRAY_H_INCLUDED_2014_04_10

/* Minimalistic smart pointer C++ class
 *
 * This smart pointer implementation has following features
 * - Reference count tracking
 * - Provides strong exception safety guarantee
 * - Arrays support (specify size in constructor)
 * - Size of array support .size()
 * - No allocation overhead if referenced only once
 * - To support ownership transfer without memory allocation for ref_count
 *   the compiler must support move semantics (C++11)
 *
 * Assumptions
 * - Not thread safe (safe inside one thread or if locks are used for synchronization)
 *
 *
 * Anyone can use it freely for any purpose. There is 
 * absolutely no guarantee it works or fits a particular purpose (see below). 
 *
 * Copyright (C) by Ruslan Yushchenko (yruslan@gmail.com)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 */

// Is noexcept supported?
#if defined(__clang__)
# if __has_feature(cxx_noexcept)
#    define NOEXCEPT noexcept
# endif // defined(__clang__)
#endif // __has_feature(cxx_noexcept)
#if	defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46
#    define NOEXCEPT noexcept
#endif // defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 180021114
#include <yvals.h>
#  ifdef _NOEXCEPT
#    define NOEXCEPT _NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif
#ifndef NOEXCEPT
#  define NOEXCEPT
#endif

// Is move semantics supported?
#if defined(__clang__)
# if __has_feature(__cxx_rvalue_references__)
#  define ENABLE_MOVE_SEMANTICS
# endif // __has_feature(__cxx_rvalue_references__)
#endif // defined(__clang__)
#if defined(__GNUC__)
# if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__)
#   define ENABLE_MOVE_SEMANTICS
#  endif // defined(__GXX_EXPERIMENTAL_CXX0X__)
# endif // ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4)
#endif // defined(__GNUC__)
#if (_MSC_VER >= 1700)
# define ENABLE_MOVE_SEMANTICS
#endif // (_MSC_VER >= 1700)

#ifdef ENABLE_MOVE_SEMANTICS
#define EXPLICIT_CONV_OPERATOR explicit
#else
#define EXPLICIT_CONV_OPERATOR
#endif

#ifndef assert
#define assert(a)
#define SMART_PTR_UNSET_ASSERT
#endif

template <typename T>
class smart_array2
{
public:
	smart_array2(T *_ptr = NULL, size_t size_ = 1) NOEXCEPT;
	smart_array2(size_t size_);
	smart_array2(const smart_array2& p);
#ifdef ENABLE_MOVE_SEMANTICS
	smart_array2(smart_array2&& p) NOEXCEPT;
#endif
	~smart_array2() NOEXCEPT;

	void reset(T *p = NULL, size_t size_ = 0) NOEXCEPT;
	T* get() const NOEXCEPT;
	void swap(smart_array2<T> &p) NOEXCEPT;

	bool unique() const NOEXCEPT;
	int use_count() const NOEXCEPT;
	size_t size() const NOEXCEPT; // returns 0 if unknown	

	smart_array2& operator=(const smart_array2& p);
#ifdef ENABLE_MOVE_SEMANTICS
	smart_array2& operator=(smart_array2&& p) NOEXCEPT;
#endif
	bool operator< (const smart_array2 &rhs) const NOEXCEPT;
	bool operator== (const smart_array2 &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T* operator->() const NOEXCEPT;
	T& operator*() const NOEXCEPT;
	T& operator[](size_t ind);
	const T& operator[](size_t ind) const;

private:
	T *ptr;                   // Pointer to the object itself
	mutable int *ref_count;   // Pointer to the reference counter. If NULL or *ref_count==0 => reference count=1
	size_t sz;                // Array size 1-single object, >1 - array, 0 - unknown

	void decrement() NOEXCEPT;
};

// Implementation

template <typename T>
inline smart_array2<T>::smart_array2(T *_ptr/* = NULL*/, size_t size_/* = 1*/) NOEXCEPT
: ptr(_ptr)
, sz(size_)
, ref_count(NULL)
{
}

template <typename T>
inline smart_array2<T>::smart_array2(size_t size_)
: ptr(NULL)
, sz(size_)
, ref_count(NULL)
{
	if (sz == 1)
		ptr = new T;
	else if (sz>1)
		ptr = new T[sz];
}

template <typename T>
inline smart_array2<T>::smart_array2(const smart_array2<T>& p)
: ptr(p.ptr)
, ref_count(p.ref_count)
, sz(p.sz)
{
	if (p.ref_count == NULL)
		p.ref_count = new int(1); // 2 references
	else
		(*p.ref_count)++; // >2 references
	ref_count = p.ref_count;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline smart_array2<T>::smart_array2(smart_array2<T>&& p) NOEXCEPT
: ptr(p.ptr)
, ref_count(p.ref_count)
, sz(p.sz)
{
	p.ptr = NULL;
	p.ref_count = NULL;
}
#endif

template <typename T>
inline smart_array2<T>::~smart_array2() NOEXCEPT
{
	decrement();
}

template <typename T>
inline T* smart_array2<T>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline bool smart_array2<T>::unique() const NOEXCEPT
{
	return ref_count == NULL || (*ref_count) == 0;
}


template <typename T>
inline int smart_array2<T>::use_count() const NOEXCEPT
{
	if (ref_count == NULL)
		return ptr == NULL ? 0 : 1;
	else
		return (*ref_count) + 1;
}

template <typename T>
inline size_t smart_array2<T>::size() const NOEXCEPT
{
	if (ptr==NULL)
		return 0;
	else
		return sz;
}

template <typename T>
inline void smart_array2<T>::swap(smart_array2<T> &p) NOEXCEPT
{
	T *temp_ptr = ptr;
	int *temp_ref_count = ref_count;
	size_t temp_sz = sz;

	ptr = p.ptr;
	ref_count = p.ref_count;
	sz = p.sz;

	p.ptr = temp_ptr;
	p.ref_count = temp_ref_count;
	p.sz = temp_sz;
}

template <typename T>
inline void smart_array2<T>::reset(T *p/*=NULL*/, size_t size_/*=0*/) NOEXCEPT
{
	if (ptr == p) return;
	smart_array2<T> temp(p, size_);
	swap(temp);
}

template <typename T>
inline smart_array2<T>& smart_array2<T>::operator=(const smart_array2<T>& p)
{
	smart_array2<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline smart_array2<T>& smart_array2<T>::operator=(smart_array2<T>&& p) NOEXCEPT
{
	swap(p);
	return *this;
}
#endif

template <typename T>
inline bool smart_array2<T>::operator< (const smart_array2 &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool smart_array2<T>::operator== (const smart_array2 &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}


template <typename T>
inline smart_array2<T>::operator bool() const NOEXCEPT
{
	return ptr!=NULL;
}

template <typename T>
inline T* smart_array2<T>::operator->() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T& smart_array2<T>::operator*() const NOEXCEPT
{
	return *ptr;
}

template <typename T>
inline T& smart_array2<T>::operator[](size_t ind)
{
	assert(ind<sz);
	return ptr[ind];
}

template <typename T>
inline const T& smart_array2<T>::operator[](size_t ind) const
{
	assert(ind<sz);
	return ptr[ind];
}

template <typename T>
inline void smart_array2<T>::decrement() NOEXCEPT
{
	if (ref_count != NULL)
	{
		if (*ref_count == 0)
		{
			delete ref_count;
smptr_del_obj:
			if (sz>1)
				delete[] ptr;
			else
				delete ptr;
		}
		else (*ref_count)--;
	}
	else
		goto smptr_del_obj;
}

#undef ENABLE_MOVE_SEMANTICS
#undef NOEXCEPT
#undef EXPLICIT_CONV_OPERATOR
#ifdef SMART_PTR_UNSET_ASSERT
#undef assert
#undef SMART_PTR_UNSET_ASSERT
#endif 

#endif // _SMART_ARRAY2_H_INCLUDED_2014_04_10
