#ifndef _SMART_ARRAY_H_INCLUDED_2015_05_12
#define _SMART_ARRAY_H_INCLUDED_2015_05_12

/* Minimalistic link-based single-threaded smart fixed size array C++ class
 *
 * This smart fixed size array implementation has following features
 * - Keeps links to sibling smart arrays instead of reference counting
 * - Provides strong exception safety guarantee
 * - No additional allocation/deallocation overhead as in shared_ptr
 *
 * Assumptions
 * - Not thread safe (safe inside one thread or if locks are used for synchronization)
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
class smart_array
{
public:
	typedef T value_type;

	smart_array() NOEXCEPT;
	smart_array(size_t size);
	smart_array(const smart_array& p);
#ifdef ENABLE_MOVE_SEMANTICS
	smart_array(smart_array&& p) NOEXCEPT;
#endif
	~smart_array() NOEXCEPT;

	void create(size_t size);
	size_t size();
	void clear() NOEXCEPT;

	T* get() const NOEXCEPT;
	void swap(smart_array& p) NOEXCEPT;

	bool unique() const NOEXCEPT;
	int use_count() const NOEXCEPT;
	
	smart_array& operator=(const smart_array& p);
#ifdef ENABLE_MOVE_SEMANTICS
	smart_array& operator=(smart_array&& p) NOEXCEPT;
#endif

	bool operator< (const smart_array &rhs) const NOEXCEPT;
	bool operator== (const smart_array &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T& operator[](size_t ind);
	const T& operator[](size_t ind) const;

private:
	T* ptr;                               // Pointer to the array itself
	size_t sz;
	mutable smart_array<T>* next;                   // Pointer to the next linked pointer
	mutable smart_array<T>* prev;                   // Pointer to the previous linked pointer

	void decrement() NOEXCEPT;
};

template <typename T>
inline smart_array<T>::smart_array() NOEXCEPT
	: ptr(NULL), sz(0)
{
	next = this;
	prev = this;
}

template <typename T>
inline smart_array<T>::smart_array(size_t size)
{
	ptr = size>0 ? new T[size] : NULL;
	//if (ptr!=NULL) printf("New %p...\n", ptr);
	sz = size;
	next = this;
	prev = this;
}

template <typename T>
inline smart_array<T>::smart_array(const smart_array<T>& p)
	: ptr(p.ptr), sz(p.sz)
{
	next = p.next;
	prev = const_cast<smart_array<T> *> (&p);
	p.next = this;
	next->prev = this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline smart_array<T>::smart_array(smart_array<T>&& p) NOEXCEPT
	: ptr(p.ptr)
	, sz(p.sz)
{
	next = p.next;
	prev = const_cast<smart_array<T> *> (&p);
	p.next = this;
	static_cast<smart_array<T> *> (next)->prev = this;
	p.decrement();
	p.next = &p;
	p.prev = &p;
}
#endif

template <typename T>
inline smart_array<T>::~smart_array() NOEXCEPT
{
	decrement();
}

template <typename T>
inline T* smart_array<T>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline bool smart_array<T>::unique() const NOEXCEPT
{
	return next == this;
}

template <typename T>
inline int smart_array<T>::use_count() const NOEXCEPT
{
	if (ptr == NULL) return 0;
	int cnt = 1;
	smart_array<T> *p = next;
	while (p != this)
	{
		cnt++;
		p = p->next;
	}
	return cnt;
}

template <typename T>
inline void smart_array<T>::create(size_t size)
{
	smart_array<T> temp( size );
	swap(temp);
}

template <typename T>
inline size_t smart_array<T>::size()
{
	return sz;
}

template <typename T>
inline void smart_array<T>::clear() NOEXCEPT
{
	smart_array<T> temp(NULL);
	swap(temp);
}

template <typename T>
inline void smart_array<T>::swap(smart_array<T>& p) NOEXCEPT
{
	T* temp_ptr1 = ptr;
	size_t temp_sz1 = sz;
	smart_array<T> *temp_next1 = next;
	smart_array<T> *temp_prev1 = prev;
	smart_array<T> *temp_next2 = p.next;
	smart_array<T> *temp_prev2 = p.prev;	

	ptr = p.ptr;
	sz = p.sz;
	p.ptr = temp_ptr1;
	p.sz = temp_sz1;

	if (temp_next2 != &p)
	{
		p.next->prev = this;
		p.prev->next = this;		
		next = temp_next2;
		prev = temp_prev2;
	}
	else
	{
		next = this;
		prev = this;
	}
	if (temp_next1!=this)
	{
		next->prev = &p;
		prev->next = &p;
		p.next = temp_next1;
		p.prev = temp_prev1;
	}
	else
	{
		p.next = &p;
		p.prev = &p;
	}
}

template <typename T>
inline smart_array<T>& smart_array<T>::operator=(const smart_array<T>& p)
{
	smart_array<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline smart_array<T>& smart_array<T>::operator=(smart_array<T>&& p) NOEXCEPT
{
	swap(p);
	p.decrement();
	p.next = &p;
	p.prev = &p;
	return *this;
}
#endif

template <typename T>
inline bool smart_array<T>::operator< (const smart_array<T> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool smart_array<T>::operator== (const smart_array<T> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline smart_array<T>::operator bool() const NOEXCEPT
{
	return ptr != NULL;
}

template <typename T>
inline T& smart_array<T>::operator[](size_t ind)
{
	assert(ind<sz);
	return ptr[ind];
}

template <typename T>
inline const T& smart_array<T>::operator[](size_t ind) const
{
	assert(ind<sz);
	return ptr[ind];
}

template <typename T>
inline void smart_array<T>::decrement() NOEXCEPT
{
	if (next == this)
	{
		//if (ptr!=NULL) printf("Delete %p...\n", ptr);
		delete [] ptr;
	}
	else
	{
		prev->next = next;
		next->prev = prev;
	}
	ptr = NULL;
}

#undef EXPLICIT_CONV_OPERATOR
#undef ENABLE_MOVE_SEMANTICS
#undef NOEXCEPT

#ifdef SMART_PTR_UNSET_ASSERT
#undef assert
#undef SMART_PTR_UNSET_ASSERT
#endif 

#endif // _SMART_ARRAY_H_INCLUDED_2015_05_12
