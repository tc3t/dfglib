#ifndef DFGBASE_XZDFB9F0
#define DFGBASE_XZDFB9F0

#include "dfgDefs.hpp"
#include "dfgAssert.hpp"

#include "buildConfig.hpp"
#include <limits> // For maxValueOfType
#include <utility> // For to_string
#include <cstdlib> // For byte swap.
#include <stdint.h> // For wchar_t numericTraits
#include <iterator>
#include "scopedCaller.hpp"
#include "rangeIterator.hpp"
#include <cmath>
#include <algorithm> // for std::max

#include "dfgBaseTypedefs.hpp"
#include "bits/byteSwap.hpp"
#include "build/utils.hpp"

#include "isValidIndex.hpp"

DFG_ROOT_NS_BEGIN
{

template <class T> struct NumericTraits {};
template <> struct NumericTraits<int8> {static const int8 maxValue = int8_max; static const int8 minValue = int8_min;};
template <> struct NumericTraits<int16> {static const int16 maxValue = int16_max; static const int16 minValue = int16_min;};
template <> struct NumericTraits<int32> {static const int32 maxValue = int32_max; static const int32 minValue = int32_min;};
template <> struct NumericTraits<int64> {static const int64 maxValue = int64_max; static const int64 minValue = int64_min;};

template <> struct NumericTraits<uint8> {static const uint8 maxValue = uint8_max; static const uint8 minValue = 0;};
template <> struct NumericTraits<uint16> {static const uint16 maxValue = uint16_max; static const uint16 minValue = 0;};
template <> struct NumericTraits<uint32> {static const uint32 maxValue = uint32_max; static const uint32 minValue = 0;};
template <> struct NumericTraits<uint64> {static const uint64 maxValue = uint64_max; static const uint64 minValue = 0;};

#if !(DFG_IS_QT_AVAILABLE) // TODO: Add robust check. This is used because at least some versions of Qt seems to have ushort == wchar_t
    template <> struct NumericTraits<wchar_t> {static const wchar_t maxValue = WCHAR_MAX; static const wchar_t minValue = WCHAR_MIN;};
#endif

// Returns minimum value of given integer type.
template <class T> inline T minValueOfType()
{
    DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true, "Only interger types are allowed.");
    return (std::numeric_limits<T>::min)();
}
// Overload to allow easy checking based on existing object:
// minValueOfType(val); instead of minValueOfType<decltype(val)>();
template <class T> inline T minValueOfType(const T&) {return minValueOfType<T>();}

// Returns maximum value of given integer type.
template <class T> inline T maxValueOfType()
{
    DFG_STATIC_ASSERT(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed.");
    return (std::numeric_limits<T>::max)();
}
template <class T> inline T maxValueOfType(const T&) {return maxValueOfType<T>();}

template <class To_T, class From_T>
bool isValWithinLimitsOfType(const From_T& val, const std::true_type, const std::true_type) // Both unsigned
{
    return (val >= minValueOfType<To_T>() && val <= maxValueOfType<To_T>());
}

template <class To_T, class From_T>
bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::false_type) // Both signed
{
    return (val >= minValueOfType<To_T>() && val <= maxValueOfType<To_T>());
}

template <class To_T, class From_T>
bool isValWithinLimitsOfType(const From_T& val, const std::true_type, const std::false_type) // To_T unsigned, From_T signed
{
    if (val < 0)
        return false;
    typedef std::make_unsigned<From_T>::type UFrom_T;
    return isValWithinLimitsOfType<To_T>(static_cast<UFrom_T>(val));
}

template <class To_T, class From_T>
bool isValWithinLimitsOfType(const From_T& val, const std::false_type, const std::true_type) // To_T signed, From_T unsigned
{
    typedef std::make_unsigned<To_T>::type UTo_T;
    return val <= static_cast<UTo_T>(maxValueOfType<To_T>());
}

// Tests whether 'val' is within [minValueOfType<To_T>(), maxValueOfType<To_T>()].
template <class To_T, class From_T>
bool isValWithinLimitsOfType(const From_T& val)
{
    return isValWithinLimitsOfType<To_T>(val, std::is_unsigned<To_T>(), std::is_unsigned<From_T>());
}

// Convenience overload to allow using variables.
template <class From_T, class To_T>
bool isValWithinLimitsOfType(const From_T& val, const To_T)
{
    return isValWithinLimitsOfType<To_T>(val);
}

// Note: Min and Max differ from normal naming to avoid problems with windows headers that
//       define min and max as macros.
template <class T> inline const T& Min(const T& v1, const T& v2) {return (std::min)(v1, v2);}
template <class T> inline const T& Min(const T& v1, const T& v2, const T& v3) {return Min(Min(v1, v2), v3);}
template <class T> inline const T& Min(const T& v1, const T& v2, const T& v3, const T& v4) {return Min(Min(v1,v2,v3), v4);}

template <class T> inline const T& Max(const T& v1, const T& v2) {return (std::max)(v1, v2);}
template <class T> inline const T& Max(const T& v1, const T& v2, const T& v3) {return Max(Max(v1, v2), v3);}
template <class T> inline const T& Max(const T& v1, const T& v2, const T& v3, const T& v4) {return Max(Max(v1,v2,v3), v4);}

// Returns the size of given container.
template<class ContT> size_t count(const ContT& cont) {return cont.size();}
template<class T, size_t N> size_t count(const T (&)[N]) {return N;}

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
template <class T, class C>
inline void limit(T& val, const C& lowerLimit, const C& upperLimit)
{
    if (lowerLimit > upperLimit) return;
    if (val < lowerLimit) val = lowerLimit;
    else if (val > upperLimit) val = upperLimit;
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
    DFG_CLASS_NAME(Dummy)(const T&) {}
};

} // module dfg Root

#include "readOnlyParamStr.hpp"


#endif // include guard
