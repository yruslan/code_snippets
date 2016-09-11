#ifndef _SHAREDPTR1_H_INCLUDED_2014_04_10
#define _SHAREDPTR1_H_INCLUDED_2014_04_10

/* Minimalistic refcount based single-threaded smart pointer C++ class
 *
 * This smart pointer implementation has following features
 * - Reference count tracking
 * - Provides strong exception safety guarantee
 * - No allocation overhead if referenced only once
 * - To support ownership transfer without memory allocation for ref_count
 *   the compiler must support move semantics (C++11)
 * - Have specialization for default deleter (takes one pointer less space)
 * - Have specialization for arrays (e.g. shared_ptr1<int[]>)
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
struct smart_ptr_deleter
{
	void operator()(T* p)
	{
		delete p;
	}
};

template <typename T>
struct smart_array_deleter
{
	void operator()(T* p)
	{
		delete [] p;
	}
};

namespace details
{
	struct shared_ptr1_deleter_interface
	{
		virtual ~shared_ptr1_deleter_interface() {};
		virtual void destroy() = 0;
	};

	template<typename T, class D>
	struct shared_ptr1_deleter_internal : public shared_ptr1_deleter_interface
	{
		shared_ptr1_deleter_internal(T* ptr_, const D &deleter)
		{
			ptr = ptr_;
			m_deleter = deleter;
		}
#ifdef ENABLE_MOVE_SEMANTICS
		shared_ptr1_deleter_internal(T* ptr_, D &&deleter)
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
	struct shared_ptr1_default_deleter : public shared_ptr1_deleter_interface
	{
		shared_ptr1_default_deleter(T* ptr_)
			: ptr(ptr_)
		{
		}
		virtual void destroy()
		{
			delete ptr;
		}
		T* ptr;
	};

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template<class T, class U>
	struct shared_ptr1_is_same {
		enum { value = 0 };
	};

	template<class T>
	struct shared_ptr1_is_same<T, T> {
		enum { value = 1 };
	};
#endif
}

template <typename T>
class shared_ptr1;

template<typename T, typename U>
shared_ptr1<T> static_cast1(shared_ptr1<U> &);

template<typename T, typename U>
shared_ptr1<T> dynamic_cast1(shared_ptr1<U> &);

template <typename T>
class shared_ptr1
{
public:
	typedef T value_type;

	explicit shared_ptr1(T* ptr_ = NULL) NOEXCEPT;
	shared_ptr1(const shared_ptr1& p);

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U> explicit shared_ptr1(U* ptr_);
	template <typename U, class D> shared_ptr1(U* ptr_, D deleter);
	template <typename U> shared_ptr1(const shared_ptr1<U>& p);
#endif

#ifdef ENABLE_MOVE_SEMANTICS
	shared_ptr1(shared_ptr1&& p) NOEXCEPT;
	template <typename U> shared_ptr1(shared_ptr1<U>&& p) NOEXCEPT;
#endif
	~shared_ptr1() NOEXCEPT;

	void reset(T* p = NULL) NOEXCEPT;
#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U>
	void reset(U* p) NOEXCEPT;
#endif
	T* get() const NOEXCEPT;
	T* release();                                     // Releases the pointer (if not unique returns NULL);
	void swap(shared_ptr1& p) NOEXCEPT;

	bool unique() const NOEXCEPT;
	int use_count() const NOEXCEPT;
	void create();                                    // Same as "shared_ptr<T> p = make_shared<T>();"

	shared_ptr1& operator=(const shared_ptr1& p);

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U>
	shared_ptr1& operator=(const shared_ptr1<U>& p);
#endif

#ifdef ENABLE_MOVE_SEMANTICS
	shared_ptr1& operator=(shared_ptr1&& p) NOEXCEPT;
	template <typename U> shared_ptr1& operator=(shared_ptr1<U>&& p) NOEXCEPT;
#endif

	bool operator< (const shared_ptr1 &rhs) const NOEXCEPT;
	bool operator== (const shared_ptr1 &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T* operator->() const NOEXCEPT;
	T& operator*() const NOEXCEPT;

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U1> friend class shared_ptr1;
#endif
	template<typename U1, typename U2>
	friend shared_ptr1<U1> static_cast1(shared_ptr1<U2>&);
	template<typename U1, typename U2>
	friend shared_ptr1<U1> dynamic_cast1(shared_ptr1<U2>&);
private:
	T* ptr;                                           // Pointer to the object itself
	mutable int* ref_count;                           // Pointer to the reference counter. If NULL or *ref_count==0 => reference count=1
	details::shared_ptr1_deleter_interface *p_del;    // Pointer to the custom deleter

	void decrement() NOEXCEPT;
};

// -------------------------------- Supplement functions ---------------------------------------

template<typename T>
shared_ptr1<T> make_shared1()
{
	return shared_ptr1<T>(new T());	
}

template<typename T, typename U>
shared_ptr1<T> static_cast1(shared_ptr1<U> &pu)
{
	shared_ptr1<T> pt;
	pt.ptr = static_cast<T *> (pu.ptr);
	if (pu.ref_count == NULL)
		pu.ref_count = new int(1);
	else
		++(*pu.ref_count); // >2 references
	pt.ref_count = pu.ref_count;
	if (pu.p_del == NULL && !details::shared_ptr1_is_same<T, U>().value)
	{
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U>(pu.ptr);
		pt.p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
	else
		pt.p_del = pu.p_del;
	return pt;
}

template<typename T, typename U>
shared_ptr1<T> dynamic_cast1(shared_ptr1<U> &pu)
{
	shared_ptr1<T> pt;
	pt.ptr = dynamic_cast<T *>(pu.ptr);
	if (pu.ref_count == NULL)
		pu.ref_count = new int(1);
	else
		++(*pu.ref_count); // >2 references
	pt.ref_count = pu.ref_count;
	if (pu.p_del == NULL && !details::shared_ptr1_is_same<T, U>().value)
	{
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U>(pu.ptr);
		pt.p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
	else
		pt.p_del = pu.p_del;
	return pt;
}

// --------------------------- Implementation of general case ----------------------------------

template <typename T>
inline shared_ptr1<T>::shared_ptr1(T* ptr_/* = NULL*/) NOEXCEPT
	: ptr(ptr_)
	, ref_count(NULL)
	, p_del(NULL)
{
}

template <typename T>
inline shared_ptr1<T>::shared_ptr1(const shared_ptr1<T>& p)
	: ptr(p.ptr)
	, ref_count(p.ref_count)
	, p_del(p.p_del)
{
	if (p.ref_count == NULL)
		p.ref_count = new int(1); // 2 references
	else
		++(*p.ref_count); // >2 references
	ref_count = p.ref_count;
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline shared_ptr1<T>::shared_ptr1(U* ptr_)
: ptr(static_cast<T*> (ptr_))
, ref_count(NULL)
, p_del(NULL)
{
	if (!details::shared_ptr1_is_same<T, U>().value)
	{
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U> (ptr_);
		p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
}

template <typename T>
template <typename U, class D>
inline shared_ptr1<T>::shared_ptr1(U* ptr_, D deleter)
: ptr(static_cast<T*> (ptr_))
, ref_count(NULL)
, p_del(NULL)
{
	details::shared_ptr1_deleter_internal<U, D> *pDel = new details::shared_ptr1_deleter_internal<U, D>(ptr_, MOVE(deleter));
	p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
}

template <typename T>
template <typename U>
inline shared_ptr1<T>::shared_ptr1(const shared_ptr1<U>& p)
: ptr(static_cast<T*> (p.ptr))
, ref_count(p.ref_count)
, p_del(p.p_del)
{
	if (p.ref_count == NULL)
		p.ref_count = new int(1); // 2 references
	else
		++(*p.ref_count); // >2 references
	ref_count = p.ref_count;
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline shared_ptr1<T>::shared_ptr1(shared_ptr1<T>&& p) NOEXCEPT
	: ptr(p.ptr)
	, ref_count(p.ref_count)
	, p_del(p.p_del)
{
	p.ptr = NULL;
	p.ref_count = NULL;
	p.p_del = NULL;
}

template <typename T>
template <typename U>
inline shared_ptr1<T>::shared_ptr1(shared_ptr1<U>&& p) NOEXCEPT
	: ptr(static_cast<T*> (p.ptr))
	, ref_count(p.ref_count)
	, p_del(p.p_del)
{
	p.ptr = NULL;
	p.ref_count = NULL;
	p.p_del = NULL;
}
#endif

template <typename T>
inline shared_ptr1<T>::~shared_ptr1() NOEXCEPT
{
	decrement();
}

template <typename T>
inline T* shared_ptr1<T>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T* shared_ptr1<T>::release()
{
	if (!unique()) return NULL;
	T* p = ptr;
	delete ref_count;
	delete p_del;
	ref_count = NULL;
	ptr = NULL;
	p_del = NULL;
	return p;
}

template <typename T>
inline bool shared_ptr1<T>::unique() const NOEXCEPT
{
	return ref_count == NULL || (*ref_count) == 0;
}

template <typename T>
inline int shared_ptr1<T>::use_count() const NOEXCEPT
{
	if (ref_count == NULL)
		return ptr == NULL ? 0 : 1;
	return (*ref_count) + 1;
}

template <typename T>
inline void shared_ptr1<T>::create()
{
	shared_ptr1<T> temp( new T );
	if (temp.get()!=NULL)
		swap(temp);
}

template <typename T>
inline void shared_ptr1<T>::swap(shared_ptr1<T>& p) NOEXCEPT
{
	T* temp_ptr = ptr;
	int* temp_ref_count = ref_count;
	details::shared_ptr1_deleter_interface *temp_del = p_del;

	ptr = p.ptr;
	ref_count = p.ref_count;
	p_del = p.p_del;

	p.ptr = temp_ptr;
	p.ref_count = temp_ref_count;
	p.p_del = temp_del;
}

template <typename T>
inline void shared_ptr1<T>::reset(T* p/*=NULL*/) NOEXCEPT
{
	if (ptr == p) return;
	shared_ptr1<T> temp(p);
	swap(temp);
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline void shared_ptr1<T>::reset(U* p) NOEXCEPT
{
	if (ptr == p) return;
	shared_ptr1<T> temp(p);
	swap(temp);
}
#endif // ENABLE_TEMPLATE_OVERLOADS

template <typename T>
inline shared_ptr1<T>& shared_ptr1<T>::operator=(const shared_ptr1<T>& p)
{
	shared_ptr1<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline shared_ptr1<T>& shared_ptr1<T>::operator=(const shared_ptr1<U>& p)
{
	shared_ptr1<T> temp(p);
	swap(temp);
	return *this;
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline shared_ptr1<T>& shared_ptr1<T>::operator=(shared_ptr1<T>&& p) NOEXCEPT
{
	swap(p);
	return *this;
}

template <typename T>
template <typename U>
inline shared_ptr1<T>& shared_ptr1<T>::operator=(shared_ptr1<U>&& p) NOEXCEPT
{
	decrement();

	ptr = static_cast<T*>(p.ptr);
	ref_count = p.ref_count;
	p_del = p.p_del;

	p.ptr = NULL;
	p.ref_count = NULL;
	p.p_del = NULL;
	return *this;
}
#endif

template <typename T>
inline bool shared_ptr1<T>::operator< (const shared_ptr1<T> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool shared_ptr1<T>::operator== (const shared_ptr1<T> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline shared_ptr1<T>::operator bool() const NOEXCEPT
{
	return ptr != NULL;
}

template <typename T>
inline T* shared_ptr1<T>::operator->() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T& shared_ptr1<T>::operator*() const NOEXCEPT
{
	return *ptr;
}

template <typename T>
inline void shared_ptr1<T>::decrement() NOEXCEPT
{
	if (ref_count != NULL)
	{
		if (*ref_count == 0)
		{
			delete ref_count;
smptr_del_obj:
			if (p_del==NULL)
				delete ptr;
			else
			{
				p_del->destroy();
				delete p_del;
			}
		}
		else (*ref_count)--;
	}
	else
		goto smptr_del_obj;
}

#ifdef ENABLE_TEMPLATE_SPECIALIZATION

// --------------------------- Specialization for array ----------------------------------

template <typename T>
class shared_ptr1 <T[]>
{
public:
	typedef T value_type;

	explicit shared_ptr1(T* ptr_ = NULL) NOEXCEPT;

	shared_ptr1(const shared_ptr1& p);
#ifdef ENABLE_MOVE_SEMANTICS
	shared_ptr1(shared_ptr1&& p) NOEXCEPT;
#endif
	~shared_ptr1() NOEXCEPT;

	void reset(T* p = NULL) NOEXCEPT;
	T* get() const NOEXCEPT;
	T* release();                                     // Releases the pointer (if not unique returns NULL);
	void swap(shared_ptr1& p) NOEXCEPT;

	bool unique() const NOEXCEPT;
	int use_count() const NOEXCEPT;
	
	shared_ptr1& operator=(const shared_ptr1& p);
#ifdef ENABLE_MOVE_SEMANTICS
	shared_ptr1& operator=(shared_ptr1&& p) NOEXCEPT;
#endif

	bool operator< (const shared_ptr1 &rhs) const NOEXCEPT;
	bool operator== (const shared_ptr1 &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T& operator[](size_t idx) NOEXCEPT;
	const T& operator[](size_t idx) const NOEXCEPT;

private:
	T* ptr;                                           // Pointer to the object itself
	mutable int* ref_count;                           // Pointer to the reference counter. If NULL or *ref_count==0 => reference count=1

	void decrement() NOEXCEPT;
};

template <typename T>
inline shared_ptr1<T[]>::shared_ptr1(T* ptr_/* = NULL*/) NOEXCEPT
	: ptr(ptr_)
	, ref_count(NULL)
{
}

template <typename T>
inline shared_ptr1<T[]>::shared_ptr1(const shared_ptr1<T[]>& p)
	: ptr(p.ptr)
	, ref_count(p.ref_count)
{
	if (p.ref_count == NULL)
		p.ref_count = new int(1); // 2 references
	else
		++(*p.ref_count); // >2 references
	ref_count = p.ref_count;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline shared_ptr1<T[]>::shared_ptr1(shared_ptr1<T[]>&& p) NOEXCEPT
	: ptr(p.ptr)
	, ref_count(p.ref_count)
{
	p.ptr = NULL;
	p.ref_count = NULL;
}
#endif

template <typename T>
inline shared_ptr1<T[]>::~shared_ptr1() NOEXCEPT
{
	decrement();
}

template <typename T>
inline T* shared_ptr1<T[]>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T* shared_ptr1<T[]>::release()
{
	if (!unique()) return NULL;
	T* p = ptr;
	delete ref_count;
	ref_count = NULL;
	ptr = NULL;
	return p;
}

template <typename T>
inline bool shared_ptr1<T[]>::unique() const NOEXCEPT
{
	return ref_count == NULL || (*ref_count) == 0;
}

template <typename T>
inline int shared_ptr1<T[]>::use_count() const NOEXCEPT
{
	if (ref_count == NULL)
	return ptr == NULL ? 0 : 1;
	return (*ref_count) + 1;
}

template <typename T>
inline void shared_ptr1<T[]>::swap(shared_ptr1<T[]>& p) NOEXCEPT
{
	T* temp_ptr = ptr;
	int* temp_ref_count = ref_count;

	ptr = p.ptr;
	ref_count = p.ref_count;

	p.ptr = temp_ptr;
	p.ref_count = temp_ref_count;
}

template <typename T>
inline void shared_ptr1<T[]>::reset(T* p/*=NULL*/) NOEXCEPT
{
	if (ptr == p) return;
	shared_ptr1<T> temp(p);
	swap(temp);
}

template <typename T>
inline shared_ptr1<T[]>& shared_ptr1<T[]>::operator=(const shared_ptr1<T[]>& p)
{
	shared_ptr1<T[]> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline shared_ptr1<T[]>& shared_ptr1<T[]>::operator=(shared_ptr1<T[]>&& p) NOEXCEPT
{
	swap(p);
	return *this;
}
#endif

template <typename T>
inline bool shared_ptr1<T[]>::operator< (const shared_ptr1<T[]> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool shared_ptr1<T[]>::operator== (const shared_ptr1<T[]> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline shared_ptr1<T[]>::operator bool() const NOEXCEPT
{
	return ptr != NULL;
}

template <typename T>
inline T& shared_ptr1<T[]>::operator[](size_t idx) NOEXCEPT
{
	return ptr[idx];
}

template <typename T>
inline const T& shared_ptr1<T[]>::operator[](size_t idx) const NOEXCEPT
{
	return ptr[idx];
}

template <typename T>
inline void shared_ptr1<T[]>::decrement() NOEXCEPT
{
	if (ref_count != NULL)
	{
		if (*ref_count == 0)
		{
			delete ref_count;
		smptr_del_obj:
			delete [] ptr;
		}
		else (*ref_count)--;
	}
	else
		goto smptr_del_obj;
}

#undef ENABLE_TEMPLATE_SPECIALIZATION
#endif // ENABLE_TEMPLATE_SPECIALIZATION

#undef ENABLE_MOVE_SEMANTICS
#undef NOEXCEPT
#undef EXPLICIT_CONV_OPERATOR
#undef MOVE

#ifdef ENABLE_TEMPLATE_OVERLOADS
#undef ENABLE_TEMPLATE_OVERLOADS
#endif

// --------------------------- Other stuff ----------------------------------

// A shortcut for RAII for fopen()
template <typename T>
struct smart_file_deleter
{
	void operator()(T* p)
	{
		if (p!=NULL)
			fclose(p);
	}
};

#endif // _SHAREDPTR1_H_INCLUDED_2014_04_10
