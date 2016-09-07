#ifndef _OPT_H_INCLUDED_2015_12_30
#define _OPT_H_INCLUDED_2015_12_30

/* Minimalistic alternative to boost::optional<>
 *
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

#include <new>
#include <stdexcept>
#include <assert.h>

// Insert code here
struct nothing_t
{
};

const nothing_t nothing;

class bad_maybe_access : public std::logic_error
{
public:
	bad_maybe_access()
		: std::logic_error("Attempted to access the value of an uninitialized optional object.")
	{}
};

class bad_maybe_error : public std::logic_error
{
public:
	bad_maybe_error()
		: std::logic_error("Attempted to set error level to 0 when value is not available.")
	{}
};

template <typename T>
class maybe
{
public:
	~maybe();

	maybe() NOEXCEPT;
	maybe(nothing_t) NOEXCEPT;
	maybe(const T& v);
#ifdef ENABLE_TEMPLATE_OVERLOADS
	template<typename U> maybe(const U& v);
	template<typename U> explicit maybe(maybe<U> const& rhs);
#endif

	maybe(bool condition, T const& v);
	maybe(maybe const& rhs);

	maybe& operator = (nothing_t) NOEXCEPT;
	maybe& operator = (const T& rhs);
	maybe& operator = (maybe const& rhs);
#ifdef ENABLE_TEMPLATE_OVERLOADS
	template<typename U> maybe& operator = (const U& rhs);
	template<typename U> maybe& operator = (maybe<U> const& rhs);
#endif

	bool operator!() const NOEXCEPT;

	bool is_nothing() const NOEXCEPT;

	T const& get() const;
	T&       get();

	T const& value() const;
	T&       value();

	T& value_or(T &v) const;
	T const& value_or(T const& def_val) const;

	//T const* operator ->() const;
	//T*       operator ->();

	void swap(maybe<T> &rhs);
	void swap(nothing_t) NOEXCEPT;

	int get_error() NOEXCEPT;
	void set_error(int error_num);       // Store error. If maybe<> has a value it will be destroyed. Must hold: error_num!=0

#ifdef ENABLE_MOVE_SEMANTICS
	maybe(T&& v);
	maybe(maybe&& rhs);
	template<typename U> explicit maybe(maybe<U> && rhs);
	maybe(bool condition, T&& v);

	maybe& operator = (T&& v);
	maybe& operator = (maybe&& rhs);
	template<typename U> maybe& operator = (maybe<U>&& rhs);

	T value_or(T && def_val) const;

	// Maybe Monad
	template<typename F>
	auto operator>>=(F&& f) -> decltype(f(T()));
#endif

	EXPLICIT_CONV_OPERATOR operator bool() const NOEXCEPT;

private:
	int err;
	char buf[sizeof(T)];
	static const bool isMaybe = true;

#ifdef ENABLE_TEMPLATE_OVERLOADS
	template <typename U1> friend class maybe;
#endif

	template<typename U> friend bool operator == (maybe<U> const& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator == (maybe<U> const& lhs, const U& rhs);
	template<typename U> friend bool operator == (const U& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator == (maybe<U> const& lhs, nothing_t) NOEXCEPT;
	template<typename U> friend bool operator == (nothing_t, maybe<U> const& rhs) NOEXCEPT;

	template<typename U> friend bool operator != (maybe<U> const& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator != (maybe<U> const& lhs, const U& rhs);
	template<typename U> friend bool operator != (const U& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator != (maybe<U> const& lhs, nothing_t) NOEXCEPT;
	template<typename U> friend bool operator != (nothing_t, maybe<U> const& rhs) NOEXCEPT;

	template<typename U> friend bool operator < (maybe<U> const& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator < (maybe<U> const& lhs, const U& rhs);
	template<typename U> friend bool operator < (const U& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator < (maybe<U> const& lhs, nothing_t) NOEXCEPT;
	template<typename U> friend bool operator < (nothing_t, maybe<U> const& rhs) NOEXCEPT;

	template<typename U> friend bool operator > (maybe<U> const& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator > (maybe<U> const& lhs, const U& rhs);
	template<typename U> friend bool operator > (const U& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator > (maybe<U> const& lhs, nothing_t) NOEXCEPT;
	template<typename U> friend bool operator > (nothing_t, maybe<U> const& rhs) NOEXCEPT;

	template<typename U> friend bool operator <= (maybe<U> const& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator <= (maybe<U> const& lhs, const U& rhs);
	template<typename U> friend bool operator <= (const U& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator <= (maybe<U> const& lhs, nothing_t) NOEXCEPT;
	template<typename U> friend bool operator <= (nothing_t, maybe<U> const& rhs) NOEXCEPT;

	template<typename U> friend bool operator >= (maybe<U> const& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator >= (maybe<U> const& lhs, const U& rhs);
	template<typename U> friend bool operator >= (const U& lhs, maybe<U> const& rhs);
	template<typename U> friend bool operator >= (maybe<U> const& lhs, nothing_t) NOEXCEPT;
	template<typename U> friend bool operator >= (nothing_t, maybe<U> const& rhs) NOEXCEPT;

	//friend maybe<int> operator>>=(maybe<int> t, maybe<int>(*f)(int));
};

template<typename T> maybe<T> make_maybe(T const& v);
template<typename T> maybe<T> make_maybe(bool condition, T const& v);
template<typename T> void swap(maybe<T>& lhs, maybe<T>& rhs);

//----------------------------------- Implementation -------------------------------------------

// Maybe Monad
/*template<typename T, typename F>
auto operator>>=(maybe<T> t, F&& f) -> decltype(f(t))
{
	typedef decltype(f(t)) RT;
	static_assert(RT::isMaybe, "F doesn't return a maybe");
	if (!t.bValid)
		return RT();
	else
		return std::forward<F>(f)(get());
};
*/

/*maybe<int> operator>>=(maybe<int> t, maybe<int>(*f)(int))
{
	typedef decltype(f(t.get())) RT;
	static_assert(RT::isMaybe, "F doesn't return a maybe");
	if (!t.bValid)
		return RT();
	else
		return f(t.get());
};*/

template <typename T>
inline maybe<T>::~maybe()
{
	if (!err)
		((T*)buf)->~T();
}

template <typename T>
inline maybe<T>::maybe() NOEXCEPT
	: err(-1)
{
}

template <typename T>
inline maybe<T>::maybe(const T& v)
	: err(0)
{
	T* tt = new(buf)T(v);
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U> 
maybe<T>::maybe(const U& v)
	: err(0)
{
	T* tt = new(buf)T(v);
}

#endif // ENABLE_TEMPLATE_OVERLOADS

template <typename T>
inline maybe<T>::maybe(nothing_t) NOEXCEPT
	: err(-1)
{
}

template <typename T>
inline maybe<T>::maybe(bool condition, T const& v)
	: err(-1)
{
	if (condition)
	{
		err = 0;
		new(buf)T(v);
	}
}

template <typename T>
inline maybe<T>::maybe(maybe<T> const& rhs)
	: err(rhs.err)
{
	if (!err)
	{
		new(buf)T(*((T*)rhs.buf));
	}

}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template <typename T>
template <typename U>
inline maybe<T>::maybe(maybe<U> const& rhs)
	: err(rhs.err)
{
	if (!err)
		new(buf)T(*((U*)rhs.buf));
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline maybe<T>::maybe(T&& v)
	: err(0)
{
	T* tt = new(buf)T(MOVE(v));
}

template <typename T>
inline maybe<T>::maybe(bool condition, T&& v)
	: err(-1)
{
	if (condition)
	{
		err = 0;
		new(buf)T(MOVE(v));
	}
}
template <typename T>
template <typename U>
inline maybe<T>::maybe(maybe<U> && rhs)
	: err(0)
{
	T* tt = new(buf)T(MOVE(*((U*)rhs.buf)));
}

template <typename T>
inline maybe<T>::maybe(maybe<T> && rhs)
	: err(rhs.err)
{
	if (!err)
	{
		new(buf)T(*(MOVE((T*)rhs.buf)));
	}
}
#endif // ENABLE_MOVE_SEMANTICS

template <typename T>
inline bool maybe<T>::is_nothing() const NOEXCEPT
{
	return err!=0;
}

template <typename T>
inline T const& maybe<T>::get() const
{
	assert(err==0);
	return *((T*)buf);
}

template <typename T>
inline T& maybe<T>::get()
{
	assert(err==0);
	return *((T*)buf);
}

template <typename T>
inline const T& maybe<T>::value() const
{
	if (err!=0)
		throw bad_maybe_access();
	return *((T*)buf);
}

template <typename T>
inline T& maybe<T>::value()
{
	if (err!=0)
		throw bad_maybe_access();
	return *((T*)buf);
}

template <typename T>
inline T& maybe<T>::value_or(T &v) const
{
	if (err==0)
		return *((T*)buf);
	else
		return v;
}

template <typename T>
inline const T& maybe<T>::value_or(const T &v) const
{
	if (err==0)
		return *((T*)buf);
	else
		return v;
}

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline T maybe<T>::value_or(T&& v) const
{
	if (err==0)
		return *((T*)buf);
	else
		return MOVE(v);
}
#endif  // ENABLE_MOVE_SEMANTICS

template <typename T>
inline void maybe<T>::swap(maybe<T> &rhs)
{
	maybe<T> temp(MOVE(rhs));
	rhs = MOVE(*this);
	*this = MOVE(temp);
}

template <typename T>
inline void maybe<T>::swap(nothing_t) NOEXCEPT
{
	*this = nothing;
}

template <typename T>
inline int maybe<T>::get_error() NOEXCEPT
{
	return err;
}

template <typename T>
inline void maybe<T>::set_error(int e)
{
	if (e != 0)
	{
		if (err == 0)
			((T*)buf)->~T();
		err = e;
	}
	else
	{
		if (err!=e)
			throw bad_maybe_error();
	}
}

//----------------------------------- Free functions -------------------------------------------

template<typename T>
inline void swap(maybe<T>& lhs, maybe<T>& rhs)
{
	lhs.swap(rhs);
};

template<typename T> inline maybe<T> make_maybe(T const& v)
{
	return maybe<T>(v);
}

template<typename T> inline maybe<T> make_maybe(bool condition, T const& v)
{
	if (condition)
		return maybe<T>(v);
	else
		return maybe<T>(nothing);
}

//----------------------------------- Operator overloads -------------------------------------------

//template <typename T>
//inline const T* maybe<T>::operator ->() const
//{
//	assert(err==0);
//	return (T*)buf;
//}
//
//template <typename T>
//inline T* maybe<T>::operator ->()
//{
//	assert(err==0);
//	return (T*)buf;
//}

template <typename T>
inline maybe<T>& maybe<T>::operator = (nothing_t)NOEXCEPT
{
	if (err==0)
		((T*)buf)->~T();
	err = -1;
}

template <typename T>
inline maybe<T>& maybe<T>::operator = (const T& rhs)
{
	if (err==0)
	{
		*((T*)buf) = rhs;
	}
	else
	{
		new(buf)T(rhs);
		err = 0;
	}
	return *this;
}

template <typename T>
inline maybe<T>& maybe<T>::operator = (maybe<T> const& rhs)
{
	if (err==0 && rhs.err==0)
	{
		*((T*)buf) = *((T*)rhs.buf);
	}
	if (err!=0 && rhs.err==0)
	{
		new(buf)T(*((T*)rhs.buf));
		err = 0;
	}
	if (err==0 && rhs.err!=0)
	{
		((T*)buf)->~T();
		err = -1;
	}
	return *this;
}

#ifdef ENABLE_TEMPLATE_OVERLOADS
template<typename T>
template<typename U>
inline maybe<T>& maybe<T>::operator = (const U& rhs)
{
	if (err==0)
	{
		*((T*)buf) = rhs;
	}
	else
	{
		new(buf)T(rhs);
		err = 0;
	}
	return *this;
}

template<typename T>
template<typename U>
inline maybe<T>& maybe<T>::operator = (maybe<U> const& rhs)
{
	if (err==0 && rhs.err==0)
	{
		*((T*)buf) = *((U*)rhs.buf);
	}
	if (err!=0 && rhs.err==0)
	{
		new(buf)T(*((U*)rhs.buf));
		err = 0;
	}
	if (err==0 && rhs.err!=0)
	{
		((T*)buf)->~T();
		err = -;
	}
	return *this;
}
#endif // ENABLE_TEMPLATE_OVERLOADS

#ifdef ENABLE_MOVE_SEMANTICS
template <typename T>
inline maybe<T>& maybe<T>::operator = (T&& rhs)
{
	if (err==0)
	{
		*((T*)buf) = MOVE(rhs);
	}
	else
	{
		new(buf)T(MOVE(rhs));
		err = 0;
	}
	return *this;
}

template <typename T>
inline maybe<T>& maybe<T>::operator = (maybe<T>&& rhs)
{
	if (err==0 && rhs.err==0)
	{
		*((T*)buf) = MOVE(*((T*)rhs.buf));
	}
	if (err!=0 && rhs.err==0)
	{
		new(buf)T(MOVE(*((T*)rhs.buf)));
		err = 0;
	}
	if (err==0 && rhs.err!=0)
	{
		((T*)buf)->~T();
		err = -1;
	}
	return *this;
}

template<typename T>
template<typename U>
inline maybe<T>& maybe<T>::operator = (maybe<U>&& rhs)
{
	if (err == 0 && rhs.err == 0)
	{
		*((T*)buf) = MOVE(*((U*)rhs.buf));
	}
	if (err != 0 && rhs.err == 0)
	{
		new(buf)T(MOVE(*((U*)rhs.buf)));
		err = 0;
	}
	if (err == 0 && rhs.err != 0)
	{
		((T*)buf)->~T();
		err = -1;
	}
	return *this;
}

// Maybe Monad
template<typename T>
template<typename F>
auto maybe<T>::operator>>=(F&& f) -> decltype(f(T()))
{
	typedef decltype(f(get())) RT;
	static_assert(RT::isMaybe, "F doesn't return a maybe");
	if (err!=0)
		return nothing;
	else
		return std::forward<F>(f)(get());
};
#endif // ENABLE_MOVE_SEMANTICS

template <typename T>
maybe<T>::operator bool() const NOEXCEPT
{
	return err==0;
}

template <typename T>
bool maybe<T>::operator!() const NOEXCEPT
{
	return is_nothing();
}

//----------------------------------- Comparison operators -------------------------------------------

// maybe<T> vs maybe<T>
template<typename T> inline bool operator == (maybe<T> const& lhs, maybe<T> const& rhs)
{
	if (lhs.is_nothing())
		return rhs.is_nothing();
	else
		return rhs.is_nothing() ? false : *((T*)lhs.buf) == *((T*)rhs.buf);
};

template<typename T> inline bool operator != (maybe<T> const& lhs, maybe<T> const& rhs)
{
	return !(lhs == rhs);
}

template<typename T> inline bool operator <  (maybe<T> const& lhs, maybe<T> const& rhs)
{
	if (lhs.is_nothing())
		return !rhs.is_nothing();
	else
		return rhs.is_nothing() ? false : *((T*)lhs.buf) < *((T*)rhs.buf);
}

template<typename T> inline bool operator >  (maybe<T> const& lhs, maybe<T> const& rhs)
{
	if (lhs.is_nothing())
		return false;
	else
		return rhs.is_nothing() ? true : *((T*)lhs.buf) > *((T*)rhs.buf);
}

template<typename T> inline bool operator <= (maybe<T> const& lhs, maybe<T> const& rhs)
{
	if (lhs.is_nothing())
		return true;
	else
		return rhs.is_nothing() ? false : *((T*)lhs.buf) <= *((T*)rhs.buf);
}

template<typename T> inline bool operator >= (maybe<T> const& lhs, maybe<T> const& rhs)
{
	if (lhs.is_nothing())
		return rhs.is_nothing();
	else
		return rhs.is_nothing() ? true : *((T*)lhs.buf) >= *((T*)rhs.buf);
}

// maybe<T> vs T

template<typename T> inline bool operator == (maybe<T> const& lhs, const T& rhs)
{
	if (lhs.is_nothing())
		return false;
	else
		return *((T*)lhs.buf) == rhs;
};

template<typename T> inline bool operator != (maybe<T> const& lhs, const T& rhs)
{
	return !(lhs == rhs);
}

template<typename T> inline bool operator < (maybe<T> const& lhs, const T& rhs)
{
	if (lhs.is_nothing())
		return true;
	else
		return *((T*)lhs.buf) < rhs;
}

template<typename T> inline bool operator > (maybe<T> const& lhs, const T& rhs)
{
	if (lhs.is_nothing())
		return false;
	else
		return *((T*)lhs.buf) > rhs;
}

template<typename T> inline bool operator <= (maybe<T> const& lhs, const T& rhs)
{
	if (lhs.is_nothing())
		return true;
	else
		return *((T*)lhs.buf) <= rhs;
}

template<typename T> inline bool operator >= (maybe<T> const& lhs, const T& rhs)
{
	if (lhs.is_nothing())
		return false;
	else
		return *((T*)lhs.buf) >= rhs;
}

// T vs maybe<T>

template<typename T> inline bool operator == (const T& lhs, maybe<T> const& rhs)
{
	return rhs == *((T*)lhs.buf);
};

template<typename T> inline bool operator != (const T& lhs, maybe<T> const& rhs)
{
	return !(lhs == rhs);
}
template<typename T> inline bool operator < (const T& lhs, maybe<T> const& rhs)
{
	return !(rhs >= lhs);
}

template<typename T> inline bool operator > (const T& lhs, maybe<T> const& rhs)
{
	return !(rhs <= lhs);
}

template<typename T> inline bool operator <= (const T& lhs, maybe<T> const& rhs)
{
	return !(rhs > lhs);
}

template<typename T> inline bool operator >= (const T& lhs, maybe<T> const& rhs)
{
	return !(rhs < lhs);
}

// maybe<T> vs nothing_t

template<typename T> inline bool operator == (maybe<T> const& lhs, nothing_t) NOEXCEPT
{
	return lhs.is_nothing();
};

template<typename T> inline bool operator != (maybe<T> const& lhs, nothing_t) NOEXCEPT
{
	return !lhs.is_nothing();
};

template<typename T> inline bool operator < (maybe<T> const& lhs, nothing_t) NOEXCEPT
{
	return false;
};

template<typename T> inline bool operator > (maybe<T> const& lhs, nothing_t) NOEXCEPT
{
	return !lhs.is_nothing();
};

template<typename T> inline bool operator <= (maybe<T> const& lhs, nothing_t) NOEXCEPT
{
	return lhs.is_nothing();
};

template<typename T> inline bool operator >= (maybe<T> const& lhs, nothing_t) NOEXCEPT
{
	return lhs.is_nothing();
};

// nothing_t vs maybe<T>

template<typename T> inline bool operator == (nothing_t, maybe<T> const& rhs) NOEXCEPT
{
	return rhs.is_nothing();
};

template<typename T> inline bool operator != (nothing_t, maybe<T> const& rhs) NOEXCEPT
{
	return !rhs.is_nothing();
};

template<typename T> inline bool operator < (nothing_t, maybe<T> const& rhs) NOEXCEPT
{
	return !rhs.is_nothing();
};

template<typename T> inline bool operator > (nothing_t, maybe<T> const& rhs) NOEXCEPT
{
	return false;
};

template<typename T> inline bool operator <= (nothing_t, maybe<T> const& rhs) NOEXCEPT
{
	return rhs.is_nothing();
};

template<typename T> inline bool operator >= (nothing_t, maybe<T> const& rhs) NOEXCEPT
{
	return rhs.is_nothing();
};


#ifdef ENABLE_TEMPLATE_SPECIALIZATION
#undef ENABLE_TEMPLATE_SPECIALIZATION
#endif // ENABLE_TEMPLATE_SPECIALIZATION

#ifdef ENABLE_TEMPLATE_OVERLOADS
#undef ENABLE_TEMPLATE_OVERLOADS
#endif

#undef ENABLE_MOVE_SEMANTICS
#undef NOEXCEPT
#undef EXPLICIT_CONV_OPERATOR
#undef MOVE

#endif // _SHAREDPTR1_H_INCLUDED_2014_04_10
