#ifndef _LINKED_H_INCLUDED_2015_01_27
#define _LINKED_H_INCLUDED_2015_01_27

/* Minimalistic link-based single-threaded smart pointer C++ class
 *
 * This smart pointer implementation has following features
 * - Keeps links to sibling smart pointers instead of reference counting
 * - Provides strong exception safety guarantee
 * - No additional allocation/deallocation overhead as in shared_ptr
 * - Have specialization for default deleter (takes one pointer less space)
 * - Have specialization for arrays (e.g. linked_ptr1<int[]>)
 *
 * Assumptions
 * - Not thread safe (safe inside one thread or if locks are used for synchronization)
 * - Custom deleter must not throw
 *
 * Drawbacks
 * - No weak pointers available
 * - No make_shared()-like helper function available
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
#include <utility>
#define EXPLICIT_CONV_OPERATOR explicit
#define MOVE(a) std::move(a)
#else
#define EXPLICIT_CONV_OPERATOR
#define MOVE(a) (a)
#endif

#define ENABLE_TEMPLATE_SPECIALIZATION
#define ENABLE_TEMPLATE_OVERLOADS

#ifdef _MSC_VER
#if (_MSC_VER < 1400)
#undef ENABLE_TEMPLATE_SPECIALIZATION
#undef ENABLE_TEMPLATE_OVERLOADS
#endif // (_MSC_VER < 1400)
#endif // _MSC_VER

template <typename T>
struct linked_ptr_deleter
{
	void operator()(T* p)
	{
		delete p;
	}
};

template <typename T>
struct linked_array_deleter
{
	void operator()(T* p)
	{
		delete [] p;
	}
};

template <typename T>
struct linked_file_deleter
{
	void operator()(T* p)
	{
		if (p!=NULL)
			fclose(p);
	}
};

template <typename T>
class linked_ptr1;

namespace details
{
	struct linked_deleter_interface
	{
		virtual ~linked_deleter_interface() {};
		virtual void destroy() = 0;
	};

	template<class T, class D>
	struct linked_deleter_internal : public linked_deleter_interface
	{
		linked_deleter_internal(T* ptr_, const D &deleter)
		{
			ptr = ptr_;
			m_deleter = deleter;
		}
#ifdef ENABLE_MOVE_SEMANTICS
		linked_deleter_internal(T* ptr_, D &&deleter)
		{
			ptr = ptr_;
			m_deleter = move(deleter);
		}
#endif // ENABLE_MOVE_SEMANTICS
		virtual void destroy()
		{
			m_deleter(ptr);
		}
		D m_deleter;
		T* ptr;
	};
	template<typename T>
	struct linked_default_deleter : public linked_deleter_interface
	{
		linked_default_deleter(T* ptr_)
		{
			ptr = ptr_;
		}
		virtual void destroy()
		{
			delete ptr;
		}
		T* ptr;
	};

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template<class T, class U>
	struct linked_is_same
	{
		enum { value = 0 };
	};

	template<class T>
	struct linked_is_same<T, T>
	{
		enum { value = 1 };
	};
#endif
}

template <typename T>
class linked_ptr1
{
public:
	typedef T value_type;

	explicit linked_ptr1(T* ptr_ = NULL) NOEXCEPT;
	linked_ptr1(const linked_ptr1& p) NOEXCEPT;

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U> explicit linked_ptr1(U* ptr_);
	template <typename U, class D> linked_ptr1(U* ptr_, D deleter);
	template <typename U> linked_ptr1(const linked_ptr1<U>& p) NOEXCEPT;
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
	linked_ptr1(linked_ptr1&& p) NOEXCEPT;
	template <typename U> linked_ptr1(linked_ptr1<U>&& p) NOEXCEPT;
#endif
	~linked_ptr1() NOEXCEPT;

	void reset(T* p = NULL) NOEXCEPT;
#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U> void reset(U* p) NOEXCEPT;
#endif
	T* get() const NOEXCEPT;
	T* release();                                     // Releases the pointer (if not unique returns NULL);
	void swap(linked_ptr1<T>& p) NOEXCEPT;

	bool unique() const NOEXCEPT;
	int use_count() const NOEXCEPT;
	void create();                                    // Simulates "shared_ptr<T> p = make_shared<T>();"

	linked_ptr1& operator=(const linked_ptr1& p);
#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U> linked_ptr1& operator=(const linked_ptr1<U>& p);
#endif // ENABLE_TEMPLATE_OVERLOADS
#ifdef ENABLE_MOVE_SEMANTICS
	linked_ptr1& operator=(linked_ptr1&& p) NOEXCEPT;
	template <typename U> linked_ptr1& operator=(linked_ptr1<U>&& p) NOEXCEPT;
#endif

	bool operator< (const linked_ptr1 &rhs) const NOEXCEPT;
	bool operator== (const linked_ptr1 &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T* operator->() const NOEXCEPT;
	T& operator*() const NOEXCEPT;

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U1> friend class linked_ptr1;
#endif

private:
	T* ptr;                                    // Pointer to the object itself
	mutable void* next;                        // Pointer to the next linked pointer
	mutable void* prev;                        // Pointer to the previous linked pointer
	details::linked_deleter_interface *p_del;  // Custom deleter

	void decrement() NOEXCEPT;
};

// --------------------------- Implementation of general case ----------------------------------

template <typename T>
inline linked_ptr1<T>::linked_ptr1(T* ptr_/* = NULL*/) NOEXCEPT
	: ptr(ptr_)
	, p_del(NULL)
{
	next = this;
	prev = this;
}

template <typename T>
inline linked_ptr1<T>::linked_ptr1(const linked_ptr1<T>& p) NOEXCEPT
: ptr(p.ptr)
, p_del(p.p_del)
{
	next = p.next;
	prev = const_cast<linked_ptr1<T> *> (&p);
	p.next = this;
	((linked_ptr1<T> *)next)->prev = this;
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline linked_ptr1<T>::linked_ptr1(U* ptr_)
: ptr(static_cast<T *>(ptr_))
, p_del(NULL)
{
	next = this;
	prev = this;

	if (!details::linked_is_same<T, U>().value)
	{
		details::linked_default_deleter<U> *pDel = new details::linked_default_deleter<U>(ptr_);
		p_del = static_cast<details::linked_deleter_interface *> (pDel);
	}
}

template <typename T>
template <typename U, class D>
inline linked_ptr1<T>::linked_ptr1(U* ptr_, D deleter)
	: ptr(static_cast<T *>(ptr_))
	, p_del(NULL)
{
	next = this;
	prev = this;

	details::linked_deleter_internal<U, D> *pDel = new details::linked_deleter_internal<U, D>(ptr_, MOVE(deleter));
	p_del = static_cast<details::linked_deleter_interface *> (pDel);
}

template <typename T>
template <typename U>
inline linked_ptr1<T>::linked_ptr1(const linked_ptr1<U>& p) NOEXCEPT
	: ptr(static_cast<T*>(p.ptr))
	, p_del(p.p_del)
{
	next = p.next;
	prev = const_cast<linked_ptr1<T> *> ((const linked_ptr1<T> *) &p);
	p.next = this;
	((linked_ptr1<T> *)next)->prev = this;
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline linked_ptr1<T>::linked_ptr1(linked_ptr1<T>&& p) NOEXCEPT
	: ptr(p.ptr)
	, p_del(p.p_del)
{
	next = (void *)p.next;
	prev = const_cast<void *> ((const void *)&p);
	p.next = this;
	static_cast<linked_ptr1<T> *> (next)->prev = this;
	p.decrement();
	p.next = &p;
	p.prev = &p;
}

template <typename T>
template <typename U>
inline linked_ptr1<T>::linked_ptr1(linked_ptr1<U>&& p) NOEXCEPT
	: ptr(static_cast<T*> (p.ptr))
	, p_del(p.p_del)
{
	next = (void *)p.next;
	prev = const_cast<void *> ((const void *)&p);
	p.next = this;
	static_cast<linked_ptr1<T> *> (next)->prev = this;
	p.decrement();
	p.next = &p;
	p.prev = &p;
}
#endif

template <typename T>
inline linked_ptr1<T>::~linked_ptr1() NOEXCEPT
{
	decrement();
}

template <typename T>
inline T* linked_ptr1<T>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T* linked_ptr1<T>::release()
{
	if (!unique()) return NULL;
	T* p = ptr;
	delete p_del;
	ptr = NULL;
	p_del = NULL;
	return p;
}

template <typename T>
inline bool linked_ptr1<T>::unique() const NOEXCEPT
{
	return next == this;
}

template <typename T>
inline int linked_ptr1<T>::use_count() const NOEXCEPT
{
	if (ptr==NULL) return 0;
	int cnt = 1;
	linked_ptr1<T> *p = (linked_ptr1<T> *) next;
	while (p!=this)
	{
		cnt++;
		p = (linked_ptr1<T> *) p->next;
	}
	return cnt;
}

template <typename T>
inline void linked_ptr1<T>::create()
{
	linked_ptr1<T> temp( new T );
	if (temp.get()!=NULL)
		swap(temp);
}

template <typename T>
inline void linked_ptr1<T>::swap(linked_ptr1<T>& p) NOEXCEPT
{
	T* temp_ptr1 = ptr;
	details::linked_deleter_interface * temp_pdel = p_del;
	linked_ptr1<T> *temp_next1 = (linked_ptr1<T> *) next;
	linked_ptr1<T> *temp_prev1 = (linked_ptr1<T> *) prev;
	linked_ptr1<T> *temp_next2 = (linked_ptr1<T> *) p.next;
	linked_ptr1<T> *temp_prev2 = (linked_ptr1<T> *) p.prev;	

	ptr = p.ptr;
	p_del = p.p_del;
	p.ptr = temp_ptr1;
	p.p_del = temp_pdel;

	if (temp_next2 != &p)
	{
		((linked_ptr1<T> *)p.next)->prev = this;
		((linked_ptr1<T> *)p.prev)->next = this;		
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
		((linked_ptr1<T> *)next)->prev = &p;
		((linked_ptr1<T> *)prev)->next = &p;
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
inline void linked_ptr1<T>::reset(T* p/*=NULL*/) NOEXCEPT
{
	if (ptr == p) return;
	linked_ptr1<T> temp(p);
	swap(temp);
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline void linked_ptr1<T>::reset(U* p) NOEXCEPT
{
	if (ptr == p) return;
	linked_ptr1<T> temp(p);
	swap(temp);
}
#endif // ENABLE_TEMPLATE_OVERLOADS

template <typename T>
inline linked_ptr1<T>& linked_ptr1<T>::operator=(const linked_ptr1<T>& p)
{
	linked_ptr1<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline linked_ptr1<T>& linked_ptr1<T>::operator=(const linked_ptr1<U>& p)
{
	linked_ptr1<T> temp(p);
	swap(temp);
	return *this;
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline linked_ptr1<T>& linked_ptr1<T>::operator=(linked_ptr1<T>&& p) NOEXCEPT
{
	swap(p);
	p.decrement();
	p.next = &p;
	p.prev = &p;
	return *this;
}

template <typename T>
template <typename U>
inline linked_ptr1<T>& linked_ptr1<T>::operator=(linked_ptr1<U>&& p) NOEXCEPT
{
	linked_ptr1<T> temp(p);
	swap(temp);
	p.decrement();
	p.next = &p;
	p.prev = &p;
	return *this;
}
#endif

template <typename T>
inline bool linked_ptr1<T>::operator< (const linked_ptr1<T> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool linked_ptr1<T>::operator== (const linked_ptr1<T> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline linked_ptr1<T>::operator bool() const NOEXCEPT
{
	return ptr != NULL;
}

template <typename T>
inline T* linked_ptr1<T>::operator->() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T& linked_ptr1<T>::operator*() const NOEXCEPT
{
	return *ptr;
}

template <typename T>
inline void linked_ptr1<T>::decrement() NOEXCEPT
{
	if (next == this)
	{
		if (p_del == NULL)
			delete (T*)ptr;
		else
		{
			p_del->destroy();
			delete p_del;
		}
	}
	else
	{
		((linked_ptr1<T> *)prev)->next = next;
		((linked_ptr1<T> *)next)->prev = prev;
	}
	ptr = NULL;
	p_del = NULL;
}

#ifdef ENABLE_TEMPLATE_SPECIALIZATION


// --------------------------- Specialization for array ----------------------------------

template <typename T>
class linked_ptr1 < T[] >
{
public:
	typedef T value_type;

	explicit linked_ptr1(T* ptr_ = NULL) NOEXCEPT;

	linked_ptr1(const linked_ptr1& p);	
#ifdef ENABLE_MOVE_SEMANTICS
	linked_ptr1(linked_ptr1&& p) NOEXCEPT;
#endif
	~linked_ptr1() NOEXCEPT;

	void reset(T* p = NULL) NOEXCEPT;
	T* get() const NOEXCEPT;
	T* release();                                     // Releases the pointer (if not unique returns NULL);
	void swap(linked_ptr1& p) NOEXCEPT;

	bool unique() const NOEXCEPT;
	int use_count() const NOEXCEPT;

	linked_ptr1& operator=(const linked_ptr1& p);
#ifdef ENABLE_MOVE_SEMANTICS
	linked_ptr1& operator=(linked_ptr1&& p) NOEXCEPT;
#endif

	bool operator< (const linked_ptr1 &rhs) const NOEXCEPT;
	bool operator== (const linked_ptr1 &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T& operator[](size_t idx) NOEXCEPT;
	const T& operator[](size_t idx) const NOEXCEPT;

private:
	T* ptr;                                           // Pointer to the object itself
	mutable linked_ptr1<T[]>* next;                   // Pointer to the next linked pointer
	mutable linked_ptr1<T[]>* prev;                   // Pointer to the previous linked pointer

	void decrement() NOEXCEPT;
};

template <typename T>
inline linked_ptr1<T[]>::linked_ptr1(T* ptr_/* = NULL*/) NOEXCEPT
	: ptr(ptr_)
{
	next = this;
	prev = this;
}

template <typename T>
inline linked_ptr1<T[]>::linked_ptr1(const linked_ptr1<T[]>& p)
	: ptr(p.ptr)
{
	next = p.next;
	prev = const_cast<linked_ptr1<T[]> *> (&p);
	p.next = this;
	next->prev = this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline linked_ptr1<T[]>::linked_ptr1(linked_ptr1<T[]>&& p) NOEXCEPT
	: ptr(p.ptr)
	, next(p.next)
	, prev(p.prev)
{
	((linked_ptr1<T[]> *)next)->prev = this;
	((linked_ptr1<T[]> *)prev)->next = this;
	p.ptr = NULL;
	p.next = &p;
	p.prev = &p;
}
#endif

template <typename T>
inline linked_ptr1<T[]>::~linked_ptr1() NOEXCEPT
{
	decrement();
}

template <typename T>
inline T* linked_ptr1<T[]>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T* linked_ptr1<T[]>::release()
{
	if (!unique()) return NULL;
	T* p = ptr;
	ptr = NULL;
	return p;
}

template <typename T>
inline bool linked_ptr1<T[]>::unique() const NOEXCEPT
{
	return next == this;
}

template <typename T>
inline int linked_ptr1<T[]>::use_count() const NOEXCEPT
{
	if (ptr == NULL) return 0;
	int cnt = 1;
	linked_ptr1<T> *p = next;
	while (p != this)
	{
		cnt++;
		p = p->next;
	}
	return cnt;
}

template <typename T>
inline void linked_ptr1<T[]>::swap(linked_ptr1<T[]>& p) NOEXCEPT
{
	T* temp_ptr1 = ptr;
	linked_ptr1<T> *temp_next1 = (linked_ptr1<T> *) next;
	linked_ptr1<T> *temp_prev1 = (linked_ptr1<T> *) prev;
	linked_ptr1<T> *temp_next2 = (linked_ptr1<T> *) p.next;
	linked_ptr1<T> *temp_prev2 = (linked_ptr1<T> *) p.prev;	

	ptr = p.ptr;
	p.ptr = temp_ptr1;

	if (temp_next2 != &p)
	{
		((linked_ptr1<T> *)p.next)->prev = this;
		((linked_ptr1<T> *)p.prev)->next = this;		
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
		((linked_ptr1<T> *)next)->prev = &p;
		((linked_ptr1<T> *)prev)->next = &p;
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
inline void linked_ptr1<T[]>::reset(T* p/*=NULL*/) NOEXCEPT
{
	if (ptr == p) return;
	linked_ptr1<T> temp(p);
	swap(temp);
}

template <typename T>
inline linked_ptr1<T[]>& linked_ptr1<T[]>::operator=(const linked_ptr1<T[]>& p)
{
	linked_ptr1<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline linked_ptr1<T[]>& linked_ptr1<T[]>::operator=(linked_ptr1<T[]>&& p) NOEXCEPT
{
	swap(p);
	return *this;
}
#endif

template <typename T>
inline bool linked_ptr1<T[]>::operator< (const linked_ptr1<T[]> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool linked_ptr1<T[]>::operator== (const linked_ptr1<T[]> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline linked_ptr1<T[]>::operator bool() const NOEXCEPT
{
	return ptr != NULL;
}

template <typename T>
inline T& linked_ptr1<T[]>::operator[](size_t idx) NOEXCEPT
{
	return ptr[idx];
}

template <typename T>
inline const T& linked_ptr1<T[]>::operator[](size_t idx) const NOEXCEPT
{
	return ptr[idx];
}

template <typename T>
inline void linked_ptr1<T[]>::decrement() NOEXCEPT
{
	if (next == this)
		delete [] ptr;
	else
	{
		prev->next = next;
		next->prev = prev;
	}
	ptr = NULL;
}


#undef ENABLE_TEMPLATE_SPECIALIZATION
#endif // ENABLE_TEMPLATE_SPECIALIZATION

#undef ENABLE_MOVE_SEMANTICS
#undef NOEXCEPT
#undef EXPLICIT_CONV_OPERATOR
#undef MOVE

#endif // _LINKED_H_INCLUDED_2015_01_27
