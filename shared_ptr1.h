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
#define override
#define nullptr NULL
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
		delete[] p;
	}
};

namespace details
{
	struct shared_ptr1_deleter_interface
	{
		virtual ~shared_ptr1_deleter_interface() {};
		virtual void destroy() = 0;
		virtual bool is_make_shared() = 0;
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
		void destroy() override
		{
			m_deleter(ptr);
		}
		bool is_make_shared() override
		{
			return false;
		};
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
		void destroy() override
		{
			delete ptr;
		}
		bool is_make_shared() override
		{
			return false;
		};
		T* ptr;
	};

	template<typename T>
	struct shared_ptr1_default_ms_deleter : public shared_ptr1_deleter_interface
	{
		shared_ptr1_default_ms_deleter(T* ptr_)
			: ptr(ptr_)
		{
		}
		void destroy() override
		{
			ptr->~T();
		}
		bool is_make_shared() override
		{
			return true;
		};
		T* ptr;
	};

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template<class T, class U>
	struct shared_ptr1_is_same {
		enum { value = 0 };
	};

	template<class T>
	struct shared_ptr1_is_same < T, T > {
		enum { value = 1 };
	};
#endif

	struct shared_ptr_details
	{
		shared_ptr_details() : ref_count(0), p_del(nullptr) {};
		shared_ptr_details(int r, shared_ptr1_deleter_interface *pd) : ref_count(r), p_del(pd) {};
		int ref_count;                          // Pointer to the reference counter. If nullptr or *ref_count==0 => reference count=1
		shared_ptr1_deleter_interface *p_del;   // Pointer to the custom deleter
	};
}

template <typename T>
class shared_ptr1;

template<typename T>
shared_ptr1<T> make_shared1();

template<typename T, typename U>
shared_ptr1<T> static_pointer_cast1(shared_ptr1<U> &);

template<typename T, typename U>
shared_ptr1<T> dynamic_pointer_cast1(shared_ptr1<U> &);

template<typename T, typename U>
shared_ptr1<T> const_pointer_cast1(const shared_ptr1<U> &);

template<typename T, typename U>
shared_ptr1<T> reinterpret_pointer_cast1(shared_ptr1<U> &);

template <typename T>
class shared_ptr1
{
public:
	typedef T value_type;

	explicit shared_ptr1(T* ptr_ = nullptr) NOEXCEPT;
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

	void reset(T* p = nullptr) NOEXCEPT;
#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U>
	void reset(U* p) NOEXCEPT;
#endif
	T* get() const NOEXCEPT;
	T* release();                                     // Releases the pointer (if not unique returns nullptr);
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
	template<typename U1>
	friend shared_ptr1<U1> make_shared1();
	template<typename U1, typename U2>
	friend shared_ptr1<U1> static_pointer_cast1(shared_ptr1<U2>&);
	template<typename U1, typename U2>
	friend shared_ptr1<U1> dynamic_pointer_cast1(shared_ptr1<U2>&);
	template<typename U1, typename U2>
	friend shared_ptr1<U1> const_pointer_cast1(const shared_ptr1<U2>&);
	template<typename U1, typename U2>
	friend shared_ptr1<U1> reinterpret_pointer_cast1(shared_ptr1<U2>&);
private:
	T* ptr;                                           // Pointer to the object itself
	mutable details::shared_ptr_details *d;           // Pointer to reference counter + custom deleter

	void decrement() NOEXCEPT;
};

// -------------------------------- Supplement functions ---------------------------------------

template<typename T>
shared_ptr1<T> make_shared1()
{
	shared_ptr1<T> po;

	size_t s1 = sizeof(details::shared_ptr_details);
	size_t s2 = sizeof(details::shared_ptr1_default_ms_deleter<T>);
	size_t sz = s1 + s2 + sizeof(T);

	char *p = (char *)::operator new (sz);
    T* pt = new (p + s1 + s2) T();
	details::shared_ptr1_default_ms_deleter<T> *pd = new (p + s1)details::shared_ptr1_default_ms_deleter<T>(pt);
	details::shared_ptr_details *pr = new(p) details::shared_ptr_details(0, pd);
	po.d = pr;
	po.ptr = pt;
	return po;
}

template<typename T, typename U>
shared_ptr1<T> static_pointer_cast1(shared_ptr1<U> &pu)
{
	shared_ptr1<T> pt;
	if (pu.ptr == nullptr)
		return pt;
	pt.ptr = static_cast<T *> (pu.ptr);
	if (pu.d == nullptr)
		pu.d = new details::shared_ptr_details(1, nullptr); // 2 references
	else
		++(pu.d->ref_count); // >2 references
	pt.d = pu.d;
	if (pu.d->p_del == nullptr && !details::shared_ptr1_is_same<T, U>().value)
	{
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U>(pu.ptr);
		pt.d->p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
	return pt;
}

template<typename T, typename U>
shared_ptr1<T> dynamic_pointer_cast1(shared_ptr1<U> &pu)
{
	shared_ptr1<T> pt;
	if (pu.ptr == nullptr)
		return pt;
	pt.ptr = dynamic_cast<T *>(pu.ptr);
	if (pu.d == nullptr)
		pu.d = new details::shared_ptr_details(1, nullptr); // 2 references
	else
		++(pu.d->ref_count); // >2 references
	pt.d = pu.d;
	if (pu.d->p_del == nullptr && !details::shared_ptr1_is_same<T, U>().value)
	{
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U>(pu.ptr);
		pt.d->p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
	return pt;
}

template<typename T, typename U>
shared_ptr1<T> const_pointer_cast1(const shared_ptr1<U> &pu)
{
	shared_ptr1<T> pt;
	if (pu.ptr == nullptr)
		return pt;
	pt.ptr = const_cast<T *>(pu.ptr);
	if (pu.d == nullptr)
		pu.d = new details::shared_ptr_details(1, nullptr); // 2 references
	else
		++(pu.d->ref_count); // >2 references
	pt.d = pu.d;
	return pt;
}

template<typename T, typename U>
shared_ptr1<T> reinterpret_pointer_cast1(shared_ptr1<U> &pu)
{
	shared_ptr1<T> pt;
	if (pu.ptr == nullptr)
		return pt;
	pt.ptr = reinterpret_cast<T *>(pu.ptr);
	if (pu.d == nullptr)
		pu.d = new details::shared_ptr_details(1, nullptr); // 2 references
	else
		++(pu.d->ref_count); // >2 references
	pt.d = pu.d;
	if (pu.d->p_del == nullptr && !details::shared_ptr1_is_same<T, U>().value)
	{
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U>(pu.ptr);
		pt.d->p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
	return pt;
}

// --------------------------- Implementation of general case ----------------------------------

template <typename T>
shared_ptr1<T>::shared_ptr1(T* ptr_/* = nullptr */) NOEXCEPT
	: ptr(ptr_)
	, d(nullptr)
{
}

template <typename T>
shared_ptr1<T>::shared_ptr1(const shared_ptr1<T>& p)
	: ptr(p.ptr)
	, d(p.d)
{
	if (p.d == nullptr)
	{
		p.d = new details::shared_ptr_details;
		p.d->ref_count = 1; // 2 references
		d = p.d;
	}
	else
		++(d->ref_count); // >2 references	
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
shared_ptr1<T>::shared_ptr1(U* ptr_)
	: ptr(static_cast<T*> (ptr_))
	, d(nullptr)
{
	if (!details::shared_ptr1_is_same<T, U>().value)
	{
		d = new details::shared_ptr_details;
		d->ref_count = 0; // 1 reference
		details::shared_ptr1_default_deleter<U> *pDel = new details::shared_ptr1_default_deleter<U>(ptr_);
		d->p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
	}
}

template <typename T>
template <typename U, class D>
shared_ptr1<T>::shared_ptr1(U* ptr_, D deleter)
	: ptr(static_cast<T*> (ptr_))
	, d(nullptr)
{
	d = new details::shared_ptr_details;
	d->ref_count = 0; // 1 reference
	details::shared_ptr1_deleter_internal<U, D> *pDel = new details::shared_ptr1_deleter_internal<U, D>(ptr_, MOVE(deleter));
	d->p_del = static_cast<details::shared_ptr1_deleter_interface *> (pDel);
}

template <typename T>
template <typename U>
shared_ptr1<T>::shared_ptr1(const shared_ptr1<U>& p)
	: ptr(static_cast<T*> (p.ptr))
	, d(p.d)
{
	if (p.d == nullptr)
	{
		p.d = new details::shared_ptr_details;
		p.d->ref_count = 1; // 2 references
		d = p.d;
	}
	else
		++(d->ref_count); // >2 references
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
shared_ptr1<T>::shared_ptr1(shared_ptr1<T>&& p) NOEXCEPT
	: ptr(p.ptr)
	, d(p.d)
{
	p.ptr = nullptr;
	p.d = nullptr;
}

template <typename T>
template <typename U>
shared_ptr1<T>::shared_ptr1(shared_ptr1<U>&& p) NOEXCEPT
	: ptr(static_cast<T*> (p.ptr))
	, d(p.d)
{
	p.ptr = nullptr;
	p.d = nullptr;
}
#endif

template <typename T>
shared_ptr1<T>::~shared_ptr1() NOEXCEPT
{
	decrement();
}

template <typename T>
T* shared_ptr1<T>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
T* shared_ptr1<T>::release()
{
	if (!unique()) return nullptr;
	T* p = ptr;
	if (d != nullptr)
		delete d->p_del;
	delete d;
	d = nullptr;
	ptr = nullptr;
	return p;
}

template <typename T>
bool shared_ptr1<T>::unique() const NOEXCEPT
{
	return d == nullptr || d->ref_count == 0;
}

template <typename T>
int shared_ptr1<T>::use_count() const NOEXCEPT
{
	if (d == nullptr)
		return ptr == nullptr ? 0 : 1;
	return d->ref_count + 1;
}

template <typename T>
void shared_ptr1<T>::create()
{
	shared_ptr1<T> temp(new T);
	if (temp.get() != nullptr)
		swap(temp);
}

template <typename T>
void shared_ptr1<T>::swap(shared_ptr1<T>& p) NOEXCEPT
{
	T* temp_ptr = ptr;
	details::shared_ptr_details* temp_d = d;

	ptr = p.ptr;
	d = p.d;

	p.ptr = temp_ptr;
	p.d = temp_d;
}

template <typename T>
void shared_ptr1<T>::reset(T* p/*=nullptr*/) NOEXCEPT
{
	if (ptr == p) return;
	shared_ptr1<T> temp(p);
	swap(temp);
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
void shared_ptr1<T>::reset(U* p) NOEXCEPT
{
	if (ptr == p) return;
	shared_ptr1<T> temp(p);
	swap(temp);
}
#endif // ENABLE_TEMPLATE_OVERLOADS

template <typename T>
shared_ptr1<T>& shared_ptr1<T>::operator=(const shared_ptr1<T>& p)
{
	shared_ptr1<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
shared_ptr1<T>& shared_ptr1<T>::operator=(const shared_ptr1<U>& p)
{
	shared_ptr1<T> temp(p);
	swap(temp);
	return *this;
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
shared_ptr1<T>& shared_ptr1<T>::operator=(shared_ptr1<T>&& p) NOEXCEPT
{
	swap(p);
	return *this;
}

template <typename T>
template <typename U>
shared_ptr1<T>& shared_ptr1<T>::operator=(shared_ptr1<U>&& p) NOEXCEPT
{
	decrement();

	ptr = static_cast<T*>(p.ptr);
	d = p.d;

	p.ptr = nullptr;
	p.d = nullptr;
	return *this;
}
#endif

template <typename T>
bool shared_ptr1<T>::operator< (const shared_ptr1<T> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
bool shared_ptr1<T>::operator== (const shared_ptr1<T> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
shared_ptr1<T>::operator bool() const NOEXCEPT
{
	return ptr != nullptr;
}

template <typename T>
T* shared_ptr1<T>::operator->() const NOEXCEPT
{
	return ptr;
}

template <typename T>
T& shared_ptr1<T>::operator*() const NOEXCEPT
{
	return *ptr;
}

template <typename T>
void shared_ptr1<T>::decrement() NOEXCEPT
{
	if (d != nullptr)
	{
		if (d->ref_count == 0)
		{
			if (d->p_del == nullptr)
			{
				delete ptr;
				delete d;
			}
			else
			{
				d->p_del->destroy();
				if (d->p_del->is_make_shared())
				{
					// The pointer was allocated using make_shared1(), it should be destroyed in a special way
					void *raw = d;
					d->p_del->~shared_ptr1_deleter_interface();
					d->~shared_ptr_details();
					:: operator delete (raw);
				}
				else
				{
					delete d->p_del;
					delete d;
				}
			}
		}
		else d->ref_count--;
	}
	else
		delete ptr;
}

#ifdef ENABLE_TEMPLATE_SPECIALIZATION

// --------------------------- Specialization for array ----------------------------------

template <typename T>
class shared_ptr1 < T[] >
{
public:
	typedef T value_type;

	explicit shared_ptr1(T* ptr_ = nullptr) NOEXCEPT;

	shared_ptr1(const shared_ptr1& p);
#ifdef ENABLE_MOVE_SEMANTICS
	shared_ptr1(shared_ptr1&& p) NOEXCEPT;
#endif
	~shared_ptr1() NOEXCEPT;

	void reset(T* p = nullptr) NOEXCEPT;
	T* get() const NOEXCEPT;
	T* release();                                     // Releases the pointer (if not unique returns nullptr);
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
shared_ptr1<T[]>::shared_ptr1(T* ptr_/* = nullptr*/) NOEXCEPT
	: ptr(ptr_)
	, ref_count(nullptr)
{
}

template <typename T>
shared_ptr1<T[]>::shared_ptr1(const shared_ptr1<T[]>& p)
	: ptr(p.ptr)
	, ref_count(p.ref_count)
{
	if (p.ref_count == nullptr)
		p.ref_count = new int(1); // 2 references
	else
		++(*p.ref_count); // >2 references
	ref_count = p.ref_count;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
shared_ptr1<T[]>::shared_ptr1(shared_ptr1<T[]>&& p) NOEXCEPT
	: ptr(p.ptr)
	, ref_count(p.ref_count)
{
	p.ptr = nullptr;
	p.ref_count = nullptr;
}
#endif

template <typename T>
shared_ptr1<T[]>::~shared_ptr1() NOEXCEPT
{
	decrement();
}

template <typename T>
T* shared_ptr1<T[]>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
T* shared_ptr1<T[]>::release()
{
	if (!unique()) return nullptr;
	T* p = ptr;
	delete ref_count;
	ref_count = nullptr;
	ptr = nullptr;
	return p;
}

template <typename T>
bool shared_ptr1<T[]>::unique() const NOEXCEPT
{
	return ref_count == nullptr || (*ref_count) == 0;
}

template <typename T>
int shared_ptr1<T[]>::use_count() const NOEXCEPT
{
	if (ref_count == nullptr)
	return ptr == nullptr ? 0 : 1;
	return (*ref_count) + 1;
}

template <typename T>
void shared_ptr1<T[]>::swap(shared_ptr1<T[]>& p) NOEXCEPT
{
	T* temp_ptr = ptr;
	int* temp_ref_count = ref_count;

	ptr = p.ptr;
	ref_count = p.ref_count;

	p.ptr = temp_ptr;
	p.ref_count = temp_ref_count;
}

template <typename T>
void shared_ptr1<T[]>::reset(T* p/*=nullptr*/) NOEXCEPT
{
	if (ptr == p) return;
	shared_ptr1<T> temp(p);
	swap(temp);
}

template <typename T>
shared_ptr1<T[]>& shared_ptr1<T[]>::operator=(const shared_ptr1<T[]>& p)
{
	shared_ptr1<T[]> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
shared_ptr1<T[]>& shared_ptr1<T[]>::operator=(shared_ptr1<T[]>&& p) NOEXCEPT
{
	swap(p);
	return *this;
}
#endif

template <typename T>
bool shared_ptr1<T[]>::operator< (const shared_ptr1<T[]> &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
bool shared_ptr1<T[]>::operator== (const shared_ptr1<T[]> &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline shared_ptr1<T[]>::operator bool() const NOEXCEPT
{
	return ptr != nullptr;
}

template <typename T>
T& shared_ptr1<T[]>::operator[](size_t idx) NOEXCEPT
{
	return ptr[idx];
}

template <typename T>
const T& shared_ptr1<T[]>::operator[](size_t idx) const NOEXCEPT
{
	return ptr[idx];
}

template <typename T>
void shared_ptr1<T[]>::decrement() NOEXCEPT
{
	if (ref_count != nullptr)
	{
		if (*ref_count == 0)
		{
			delete ref_count;
		smptr_del_obj:
			delete[] ptr;
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
		if (p != nullptr)
			fclose(p);
	}
};

#endif // _SHAREDPTR1_H_INCLUDED_2014_04_10
