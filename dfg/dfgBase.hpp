#ifndef DFGBASE_XZDFB9F0
#define DFGBASE_XZDFB9F0

#include "dfgDefs.hpp"
#include "dfgAssert.hpp"

#include <utility> // For to_string
#include <cstdlib> // For byte swap.
#include <stdint.h> // For wchar_t numericTraits
#include <iterator>
#include "scopedCaller.hpp"
#include "rangeIterator.hpp"
#include <cmath>

#include "dfgBaseTypedefs.hpp"
#include "bits/byteSwap.hpp"
#include "build/utils.hpp"

#include "isValidIndex.hpp"
#include "numericTypeTools.hpp"
#include "SzPtr.hpp"

DFG_ROOT_NS_BEGIN
{

// Returns the size of given container.
template<class ContT> size_t count(const ContT& cont) {return cont.size();}
template<class T, size_t N> size_t count(const T (&)[N]) {return N;}

template<class Iterable_T>  bool isEmpty(const Iterable_T& iterable) { return iterable.empty(); }
template<class T, size_t N> bool isEmpty(const T(&)[N])              { return N == 0;           }

//template <class ContT> typename ContT::const_iterator cbegin(const ContT& cont) {return cont.cbegin();}
//template <class ContT> typename ContT::const_iterator cend(const ContT& cont) {return cont.cend();}
template <class ContT> auto cbegin(const ContT& cont) -> decltype(cont.cbegin()) { return cont.cbegin(); }
template <class ContT> auto cend(const ContT& cont) -> decltype(cont.cend()) { return cont.cend(); }

template <class T, size_t N> const T* cbegin(const T (&array)[N]) {return array;}
template <class T, size_t N> const T* cend(const T (&array)[N]) {return array + N;}

template <class Iter_T>
inline bool isAtEnd(const Iter_T& iter, const Iter_T& end)
{
    return iter == end;
}

// Like size()-method in std::vector<T>, but returns size in bytes for containers that support access
// to contiguous data array through data()-method. The return value is essentially size() * sizeof(value_type).
template <class Cont_T> size_t sizeInBytes(const Cont_T& cont)
{
    DFG_STATIC_ASSERT(sizeof(typename Cont_T::value_type) == sizeof(*cont.data()), "Unexpected value type size");
    return cont.size() * sizeof(decltype(*cont.data()));
}

template <class T, size_t N> size_t sizeInBytes(const T (&)[N])
{
    return N * sizeof(T);
}

/*
  Returns running index from array-like (x, y) pair.
          0   1   2   3 col
        ---------------
     0 |  0   1   2   3
     1 |  4   5   6   7
     2 |  8   9  10  11
     3 | 12  13  14  15
     4 | 16  17  18  19 linear element index.
     row

  Note: This function does not check integer overflows (caller responsibility).
*/
template <class IntT> IntT pairIndexToLinear(const IntT nRow, const IntT nCol, const IntT nColWidth)
{
    return nRow * nColWidth + nCol;
}

// Returns (const) reference to first object of container.
// Precondition: Container must be non-empty.
// TODO: Add debug assertions for emptyness.
template <class T, size_t N> T& firstOf(T (&arr)[N]) {return arr[0];}
template <class T, size_t N> const T& firstOf(const T (&arr)[N]) {return arr[0];}
template <class ContT> typename ContT::value_type& firstOf(ContT& cont) {return *cont.begin();}
template <class ContT> const typename ContT::value_type& firstOf(const ContT& cont) {return *cont.begin();}

// Returns (const) reference to last object of container.
// Precondition: Container must be non-empty.
// TODO: Add debug assertions for emptyness.
// TODO: Check whether these should return rbegin instead of back.
template <class T, size_t N> T& lastOf(T (&arr)[N]) {return arr[N-1];}
template <class T, size_t N> const T& lastOf(const T (&arr)[N]) {return arr[N-1];}
template <class ContT> typename ContT::value_type& lastOf(ContT& cont) {return cont.back();}
template <class ContT> const typename ContT::value_type& lastOf(const ContT& cont) {return cont.back();}
template <class T, size_t N> const T& lastOfStrLiteral(const T(&arr)[N]) { DFG_STATIC_ASSERT(N >= 2, "Accessing character in empty string literal");  return arr[N - 2]; }

// Returns i'th element before the last element.
// Precondition: nFromLast < cont.size().
// Example: elementBeforeLast(container, 0) == last element of given container.
// Example2: elementBeforeLast(container, 1) == one before last element of given container.
// TODO: Add index check asserts
// TODO: Implement also for non-random access containers.
template <class ContT> typename ContT::value_type& elementBeforeLast(ContT& cont, const size_t nFromLast)
{
    return *(cont.end() - 1 - nFromLast);
    //return cont[count(cont) - 1 - nFromLast];
}
template <class ContT> const typename ContT::value_type& elementBeforeLast(const ContT& cont, const size_t nFromLast)
{
    return *(cont.end() - 1 - nFromLast);
    //return cont[count(cont) - 1 - nFromLast];
}

// Rounds value to closest integer and in case of halfway cases, rounds away from zero.
// The halfway behaviour is that mentioned in
// cppreference.com (http://en.cppreference.com/w/cpp/numeric/math/round)
// for std::round.
inline double round(const double& val) {return (val >= 0) ? std::floor(val + 0.5) : std::ceil(val - 0.5);}

// Returns floor-value casted to integer type Int_T.
// TODO: test
template <class Int_T>
inline Int_T floorToInteger(const double& val)
{
    return static_cast<Int_T>(std::floor(val));
}

// Returns ceil-value casted to integer type Int_T.
// TODO: test
template <class Int_T>
inline Int_T ceilToInteger(const double& val)
{
    return static_cast<Int_T>(std::ceil(val));
}

// Rounds given value and casts it to target integer type.
template <class T> inline T round(const double& val)
{
    static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
    const double valRounded = round(val);
    const T intval = static_cast<T>(valRounded);
    return intval;
}

// Limits 'val' to given range. If 'val' is less than 'lowerLimit', 'val' is set to value 'lowerLimit'.
// Similarly if 'val' is greater than 'upperLimit', 'val' is set to value 'upperLimit'.
// If 'lowerLimit' > 'upperLimit', 'val' won't be modified.
// If any of the input values are NaN's, behaviour is undefined.
template <class T, class C>
inline T& limit(T& val, const C lowerLimit, const C upperLimit)
{
    if (lowerLimit > upperLimit)
        return val;
    const auto effectiveLower = saturateCast<T>(lowerLimit);
    const auto effectiveUpper = saturateCast<T>(upperLimit);
    if (val < effectiveLower)
        val = effectiveLower;
    else if (val > effectiveUpper)
        val = effectiveUpper;
    return val;
}

// Like limit(), but does not modify input value and returns the limited value.
template <class T, class C>
inline T limited(T val, const C lowerLimit, const C upperLimit)
{
    limit(val, lowerLimit, upperLimit);
    return val;
}

// Like Limit, but with upperlimit only.
template <class T, class C>
inline void limitMax(T& val, const C& upperLimit)
//-----------------------------------------------
{
    if (val > upperLimit)
        val = upperLimit;
}

// Memsets given object to zero.
// Note: This function may be subject to optimization that removes the call.
//       (To be sure that the memory gets zeroed, call memsetZeroSecure)
template <class T> inline void memsetZero(T& a)
{
    static_assert(std::is_pod<T>::value == true && std::is_pointer<T>::value == false, "Won't memset non-pods or pointers");
    memset(&a, 0, sizeof(a));
}

enum CaseSensitivity
{
    CaseSensitivityYes, CaseSensitivityNo
};

// Test pointer or 'pointer-like'-object for nullness.
// TODO: test
template <class T> bool isNonNull(const T& obj)
{
    DFG_STATIC_ASSERT(std::is_pointer<decltype(obj.get())>::value, "Implementation expect object to have get-function that returns pointer-type.");
    return isNonNull(obj.get());
}
template <class T> bool isNonNull(const T* p) {return p != nullptr;}

template <class T> bool isNull(const T& obj) {return !IsNonNull(obj);}
template <class T> bool isNull(const T* p) {return !IsNonNull(p);}

// TODO: test
// TODO: add policies
template <class Target_T, class Source_T>
Target_T numericCast(const Source_T& src)
{
    return static_cast<Target_T>(src);
}

// Dereferences pointers and any pointer-like objects like shared_ptr's.
// Rationale: Using this instead of straight dereferencing provides an easy switch to
//			  check for nullptr dereferencing also in release builds.
// TODO: Should this work also for std::reference_wrapper<T>?
template <class T> auto deRef(T& p) -> decltype(*p)
{
    DFG_ASSERT_UB(p != nullptr);
    return *p;
}

class DFG_CLASS_NAME(Dummy)
{
public:
    template <class T>
    constexpr DFG_CLASS_NAME(Dummy)(const T&) {}
};

// stringLiteralByCharType(): helper for DFG_STRING_LITERAL_BY_CHARTYPE
template <class T> inline const T* stringLiteralByCharType(const char*, const wchar_t*, const char16_t*, const char32_t*)                 { DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "stringLiteralByCharType is not implemented for given type") }
template <> inline const char*     stringLiteralByCharType<char>(const char* psz, const wchar_t*, const char16_t*, const char32_t*)       { return psz; }
template <> inline const wchar_t*  stringLiteralByCharType<wchar_t>(const char*, const wchar_t* pwsz, const char16_t*, const char32_t*)   { return pwsz; }
template <> inline const char16_t* stringLiteralByCharType<char16_t>(const char*, const wchar_t*, const char16_t* psz16, const char32_t*) { return psz16; }
template <> inline const char32_t* stringLiteralByCharType<char32_t>(const char*, const wchar_t*, const char16_t*, const char32_t* psz32) { return psz32; }

namespace DFG_DETAIL_NS
{
    template <class T> inline const T* stringLiteralByCharTypeNoChar16OrChar32(const char*, const wchar_t*)                 { DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "stringLiteralByCharTypeNoChar16OrChar32 is not implemented for given type") }
    template <> inline const char*     stringLiteralByCharTypeNoChar16OrChar32<char>(const char* psz, const wchar_t*)       { return psz; }
    template <> inline const wchar_t*  stringLiteralByCharTypeNoChar16OrChar32<wchar_t>(const char*, const wchar_t* pwsz)   { return pwsz; }
}

/* Returns typed string literal by char type. To be used e.g. in template functions where one needs string literal by template parameter char type.
    Example:
        template <class Char_T> std::basic_string<Char_T> getStr() { return DFG_STRING_LITERAL_BY_CHARTYPE(Char_T, "abc"); }
*/
#if !defined(_MSC_VER) || (DFG_MSVC_VER >= DFG_MSVC_VER_2015) // u and U prefixes are supported in MSVC only since MSVC2015
    #define DFG_STRING_LITERAL_BY_CHARTYPE(CHAR_T, STR) ::DFG_ROOT_NS::stringLiteralByCharType<CHAR_T>(STR, DFG_STRING_LITERAL_TO_WSTRING_LITERAL(STR), DFG_STRING_LITERAL_TO_CHAR16_LITERAL(STR), DFG_STRING_LITERAL_TO_CHAR32_LITERAL(STR))
#else
    #define DFG_STRING_LITERAL_BY_CHARTYPE(CHAR_T, STR) ::DFG_ROOT_NS::DFG_DETAIL_NS::stringLiteralByCharTypeNoChar16OrChar32<CHAR_T>(STR, DFG_STRING_LITERAL_TO_WSTRING_LITERAL(STR))
#endif

} // module dfg Root

#include "ReadOnlySzParam.hpp"


#endif // include guard
