#ifndef _OWNPTR1_H_INCLUDED_2015_10_07
#define _OWNPTR1_H_INCLUDED_2015_10_07

/* Minimalistic smart pointer C++ class for holding polymorphic objects through base class
 *
 * This smart pointer implementation has following features
 * - Performs deep object copy on smart pointer copy
 *
 * Assumptions
 * - Not thread safe (safe inside one thread or if locks are used for synchronization)
 *
 * Drawbacks
 * - Does not have custom deleter (just uses delete)
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
#ifdef _MSC_VER
#if (_MSC_VER < 1400)
#undef ENABLE_TEMPLATE_SPECIALIZATION
#endif // (_MSC_VER < 1400)
#endif // _MSC_VER

namespace details
{
	template <typename T>
	struct own_ptr_copy_deleter_interface
	{
		virtual ~own_ptr_copy_deleter_interface() {};
		virtual void destroy() = 0;
		virtual own_ptr_copy_deleter_interface* clone() = 0;
		virtual T* get_ptr() = 0;
	};

	template<typename B, typename T>
	struct own_ptr_copy_deleter_default : public own_ptr_copy_deleter_interface<B>
	{
		explicit own_ptr_copy_deleter_default(T* ptr_)
		{
			ptr = ptr_;
		}
		virtual own_ptr_copy_deleter_interface<B>* clone()
		{
			T* ptr_new = new T(*ptr);
			own_ptr_copy_deleter_default *ptr_cloner = new own_ptr_copy_deleter_default<B, T>(ptr_new);
			return ptr_cloner;
		}
		virtual B* get_ptr()
		{
			return static_cast<B*>(ptr);
		}
		virtual void destroy()
		{
			delete ptr;
		}
		T* ptr;
	};
}


template<typename T>
class own_ptr
{
public:
	typedef T value_type;

	own_ptr();
	explicit own_ptr(T* ptr_);
	own_ptr(const own_ptr& p);
	~own_ptr() NOEXCEPT;

#ifdef ENABLE_TEMPLATE_SPECIALIZATION
	// Note. Must pass a non-const pointer, because we transfer ownership to the 
	// own_ptr object. It will use 'delete' later, so it must have a non-const access.
	template <typename U> explicit own_ptr(U* ptr_);

	// Note. Const is ok, because we will create a brand new [deep] copy of original
	// object.
	template <typename U> own_ptr(const own_ptr<U>& p);
#endif

#ifdef ENABLE_MOVE_SEMANTICS
	own_ptr(own_ptr&& p) NOEXCEPT;
	template <typename U> own_ptr(own_ptr<U>&& p) NOEXCEPT;
#endif

	void reset(T* p = NULL);
#ifdef ENABLE_TEMPLATE_SPECIALIZATION
	template <typename U> void reset(U* p);
#endif
	T* get() const NOEXCEPT;
	void swap(own_ptr& p) NOEXCEPT;

	own_ptr& operator=(const own_ptr& p);

#ifdef ENABLE_TEMPLATE_SPECIALIZATION
	template <typename U>
	own_ptr& operator=(const own_ptr<U>& p);
#endif

#ifdef ENABLE_MOVE_SEMANTICS
	own_ptr& operator=(own_ptr&& p) NOEXCEPT;
	template <typename U> own_ptr& operator=(own_ptr<U>&& p) NOEXCEPT;
#endif

	bool operator< (const own_ptr &rhs) const NOEXCEPT;
	bool operator== (const own_ptr &rhs) const NOEXCEPT;
	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;
	T* operator->() const NOEXCEPT;
	T& operator*() const NOEXCEPT;

#ifdef ENABLE_TEMPLATE_SPECIALIZATION
	template <typename U1> friend class own_ptr;
#endif

private:
	T* ptr;                                           // Pointer to the object itself
	details::own_ptr_copy_deleter_interface<T> *p_del;    // Pointer to the custom deleter

	void destroy() NOEXCEPT;
};

template <typename T>
inline own_ptr<T>::own_ptr()
	: ptr(NULL)
	, p_del(NULL)
{
}

template <typename T>
inline own_ptr<T>::own_ptr(T* ptr_/* = NULL*/)
	: ptr(ptr_)
	, p_del(NULL)
{
	if (ptr!=NULL)
	{
		p_del = new details::own_ptr_copy_deleter_default<T, T>(ptr);
	}
}

template <typename T>
inline own_ptr<T>::~own_ptr() NOEXCEPT
{
	destroy();
}

template <typename T>
inline own_ptr<T>::own_ptr(const own_ptr& p)
	: ptr(NULL)
	, p_del(NULL)
{
	if (p.p_del != NULL)
	{
		p_del = p.p_del->clone();
		ptr = p_del->get_ptr();
	}
}
#ifdef ENABLE_TEMPLATE_SPECIALIZATION
template <typename T>
template <typename U>
inline own_ptr<T>::own_ptr(U* ptr_)
	: ptr(NULL)
	, p_del(NULL)
{
#ifdef ENABLE_MOVE_SEMANTICS
	static_assert(std::is_base_of<T, U>::value, "Object must be derived from own_ptr base class");
#endif // ENABLE_MOVE_SEMANTICS
	if (ptr_ != NULL)
	{
		p_del = new details::own_ptr_copy_deleter_default<T, U>(ptr_);
		ptr = p_del->get_ptr();
	}
}

template <typename T>
template <typename U>
inline own_ptr<T>::own_ptr(const own_ptr<U>& p)
	: ptr(NULL)
	, p_del(NULL)
{
#ifdef ENABLE_MOVE_SEMANTICS
	static_assert(std::is_base_of<T, U>::value, "Object must be derived from own_ptr base class");
#endif // ENABLE_MOVE_SEMANTICS
	if (p.p_del != NULL)
	{
		p_del = static_cast<T*> (p.p_del->clone());
		ptr = p_del->get_ptr();
	}
}
#endif // ENABLE_TEMPLATE_SPECIALIZATION

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline own_ptr<T>::own_ptr(own_ptr&& p) NOEXCEPT
	: ptr(p.ptr)
	, p_del(p.p_del)
{
	p.ptr = NULL;
	p_del = NULL;
}

template <typename T>
template <typename U> 
inline own_ptr<T>::own_ptr(own_ptr<U>&& p) NOEXCEPT
	: ptr(static_cast<T*> (p.ptr))
	, p_del(p.p_del)
{
	p.ptr = NULL;
	p_del = NULL;
}

#endif // ENABLE_MOVE_SEMANTICS

template <typename T>
inline void own_ptr<T>::reset(T* p /*= NULL*/)
{
	if (ptr == p) return;
	own_ptr<T> temp(p);
	swap(temp);
}

#ifdef ENABLE_TEMPLATE_SPECIALIZATION
template <typename T>
template <typename U>
inline void own_ptr<T>::reset(U* p)
{
	if (ptr == p) return;
	own_ptr<T> temp(p);
	swap(temp);
}
#endif // ENABLE_TEMPLATE_SPECIALIZATION

template <typename T>
inline T* own_ptr<T>::get() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline void own_ptr<T>::swap(own_ptr& p) NOEXCEPT
{
	T* temp_ptr = ptr;
	details::own_ptr_copy_deleter_interface<T> *temp_del = p_del;

	ptr = p.ptr;
	p_del = p.p_del;

	p.ptr = temp_ptr;
	p.p_del = temp_del;
}

template <typename T>
inline own_ptr<T>& own_ptr<T>::operator=(const own_ptr& p)
{
	own_ptr<T> temp(p);
	swap(temp);
	return *this;
}

#ifdef ENABLE_TEMPLATE_SPECIALIZATION
template <typename T>
template <typename U>
inline own_ptr<T>& own_ptr<T>::operator=(const own_ptr<U>& p)
{
	own_ptr<T> temp(p);
	swap(temp);
	return *this;
}
#endif

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline own_ptr<T>& own_ptr<T>::operator=(own_ptr&& p) NOEXCEPT
{
	swap(p);
	return *this;
}

template <typename T>
template <typename U>
inline own_ptr<T>& own_ptr<T>::operator=(own_ptr<U>&& p) NOEXCEPT
{
	destroy();

	ptr = static_cast<T*>(p.ptr);
	p_del = p.p_del;

	p.ptr = NULL;
	p.p_del = NULL;
	return *this;
}
#endif

template <typename T>
inline bool own_ptr<T>::operator< (const own_ptr &rhs) const NOEXCEPT
{
	return ptr < rhs.ptr;
}

template <typename T>
inline bool own_ptr<T>::operator== (const own_ptr &rhs) const NOEXCEPT
{
	return ptr == rhs.ptr;
}

template <typename T>
inline own_ptr<T>::operator bool() const NOEXCEPT
{
	return ptr != NULL;
}

template <typename T>
inline T* own_ptr<T>::operator->() const NOEXCEPT
{
	return ptr;
}

template <typename T>
inline T& own_ptr<T>::operator*() const NOEXCEPT
{
	return *ptr;
}

template <typename T>
inline void own_ptr<T>::destroy() NOEXCEPT
{
	if (p_del==NULL)
	{
		delete ptr;
	}
	else
	{
		p_del->destroy();
		delete p_del;
	}
	ptr = NULL;
	p_del = NULL;
}

#undef ENABLE_MOVE_SEMANTICS
#undef NOEXCEPT
#undef EXPLICIT_CONV_OPERATOR
#undef MOVE

#ifdef ENABLE_TEMPLATE_SPECIALIZATION
#undef ENABLE_TEMPLATE_SPECIALIZATION
#endif

#endif // _OWNPTR1_H_INCLUDED_2015_10_07
