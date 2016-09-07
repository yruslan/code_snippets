#include <stdio.h>
#include <string>
#include <set>
#include <math.h>
#include "maybe.h"

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

static int nTestNum = 0;

#define TEST(a) { nTestNum ++; \
	if (!(a)) {printf("Test %d FAILED! (%s)\n", nTestNum, #a); return nTestNum; } }

static maybe<std::string> GetOptString(bool bReturnString)
{
	if (bReturnString)
		return "This is a test string";
	else
		return nothing;
}

#ifdef ENABLE_MOVE_SEMANTICS
static maybe<int> EvenDiv2(int v)
{
	if ((v % 2) == 0)
		return v / 2;
	else
		return nothing;
}
static maybe<int> EvenStrLen(const std::string &str)
{
	int len = (int)str.size();
	if ((len % 2) == 0)
		return len;
	else
		return nothing;
}
#endif

int maybe_test()
{
	nTestNum = 0;

	// Constructors
	maybe<int> i0(10);
	maybe<int> i1(nothing);
	maybe<int> i2(i0);
	maybe<int> i3(i0.get());
	maybe<long> i4(i3);
	maybe<int> i5(bool(true));
	maybe<int> i6 = make_maybe(true, 6);
	maybe<int> i7 = make_maybe(false, 7);
	maybe<int> i8;

	TEST(i0 == 10);
	TEST(i0);
	TEST(!i0.is_nothing());
	TEST(i1 == nothing);
	TEST(!i1);
	TEST(i1.is_nothing());
	TEST(i6);
	TEST(!i7);
	TEST(i2 == 10);
	TEST(i3 == 10);
	TEST(i4.get() == 10);
	TEST(i5 == 1);
	TEST(i6 == 6);

	// Functions
	swap(i0, i1);
	TEST(i1 == 10);
	TEST(i1);
	TEST(i0 == nothing);
	TEST(!i0);

	// Operators
	i3 = i1;
	TEST(i3.get() == 10);
	TEST(i3 == 10);
	TEST(i3 > 9);
	TEST(i3 >= 9);
	TEST(i3 >= 10);
	TEST(i3 < 11);
	TEST(i3 <= 11);

	TEST(!(i3 < 9));
	TEST(!(i3 <= 9));
	TEST(!(i3 != 10));
	TEST(!(i3 > 11));
	TEST(!(i3 >= 11));

	TEST(i1 == i3);
	TEST(i1 <= i3);
	TEST(i1 >= i3);
	i1 = 9;
	TEST(i1 != i3);
	TEST(i1 < i3);
	TEST(i1 <= i3);
	TEST(i3 > i1);
	TEST(i3 >= i1);
	TEST(i3 >= i1);
	TEST(!(i1 == i3));
	TEST(!(i3 < i1));
	TEST(!(i3 <= i1));
	TEST(!(i1 > i3));
	TEST(!(i1 >= i3));
	TEST(i8 < 0);
	TEST(i8 < -9999);
	TEST(i8 <= 0);
	TEST(i8 <= -9999);
	TEST(!(i8 > 0));
	TEST(!(i8 > -9999));
	TEST(!(i8 >= 0));
	TEST(!(i8 >= -9999));
	TEST(i8 == nothing);
	TEST(!(i8 != nothing));

	i3 = i0;
	TEST(!i3);
	TEST(i3.value_or(5) == 5);

	// String
	maybe<std::string> str1 = GetOptString(true);
	maybe<std::string> str2 = GetOptString(false);

	TEST(str1);
	TEST(!str2);
	TEST(str1.get() == "This is a test string");

	// Set of strings
	std::set< maybe<std::string> > setStr;
	setStr.insert(std::string("test1"));
	setStr.insert(std::string("test2"));
	setStr.insert(std::string("test3"));
	setStr.insert(nothing);

	std::set< maybe<std::string> >::iterator it1 = setStr.find(std::string("test1"));
	std::set< maybe<std::string> >::iterator it2 = setStr.find(std::string("test8"));
	std::set< maybe<std::string> >::iterator it3 = setStr.find(nothing);

	TEST((*it1).get() == "test1");
	TEST(it2 == setStr.end());
	TEST(!(*it3));

	// Structures
	struct stx
	{
		int a;
		float b;
	};
	maybe<stx> c = stx();
	c.get().a = 10;
	c.get().b = 55.1f;
	TEST(c.get().a == 10);
	TEST(fabs(c.get().b - 55.1f) < 0.01);

	// Exceptions
	try
	{
		int i3_v = i3.value();
		assert(false);
	}
	catch (bad_maybe_access)
	{
		assert(true);
	}

	// Maybe Monad
#ifdef ENABLE_MOVE_SEMANTICS
	// EvenDiv2(n): if n is even return n/2, else return nothing
	maybe<int> intnum = 20;
	// 20/2 -> 10
	auto res1 = EvenDiv2(intnum.get());
	// (20/2)/2 -> 5
	auto res2 = (intnum >>= EvenDiv2) >>= EvenDiv2;
	// ((20 / 2) / 2) / 2 -> nothing
	auto res3 = ((intnum >>= EvenDiv2) >>= EvenDiv2) >>= EvenDiv2;
	TEST(res1);
	TEST(res1.get()==10);
	TEST(res2);
	TEST(res2==5);
	TEST(!res3);

	// EvenStrLen(s): if length of s is even return len(s), else return nothing
	maybe<std::string> sMaybeStr6 = std::string("string");
	maybe<std::string> sMaybeStr62 = "string";
	maybe<std::string> sMaybeStr7;
	TEST(!sMaybeStr7);
	sMaybeStr7 = "string7";
	TEST(sMaybeStr7);
	TEST(sMaybeStr7 == std::string("string7"));
	auto res4 = (sMaybeStr6 >>= EvenStrLen);
	auto res5 = (sMaybeStr7 >>= EvenStrLen);
	TEST(res4);
	TEST(res4.get() == 6);
	TEST(!res5);
	res3.set_error(20);
	TEST(!res3);
	TEST(res3.get_error()==20);
#endif

	return 0;
}
