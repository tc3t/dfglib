#ifndef DFG_RAND_EUBTGCDX
#define DFG_RAND_EUBTGCDX

#include "dfgBase.hpp"
#include "buildConfig.hpp"
#include "dfgAssert.hpp"
#include <type_traits>

#if DFG_LANGFEAT_HAVE_CPP11RAND // Not accurate: also needs C++11 type_traits

#include <random>
#include <limits>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(rand) {


// Wraps type for uniform distribution for element type T.
template <class T> struct DFG_CLASS_NAME(DistributionTypeUniform)
//===============================================
{
public:

#if (DFG_MSVC_VER==DFG_MSVC_VER_2010)

    // Workaround for buggy uniform_int distribution in VC2010
    // http://connect.microsoft.com/VisualStudio/feedback/details/712984
    // "std::uniform_int_distribution produces incorrect results when min0 is negative"
    template <class T2> class UniformIntDistributionImpl : public std::uniform_int_distribution<T2>
    {
    public:
        typedef std::uniform_int_distribution<T2> BaseClass;
        typedef typename BaseClass::result_type result_type;

        explicit UniformIntDistributionImpl(result_type min0 = 0, result_type max0 = (std::numeric_limits<T2>::max)()) :
            BaseClass((min0 < 0 && min0 > std::numeric_limits<result_type>::min()) ? min0 - 1 : min0, max0) // Note: Won't work correctly if min0 == std::numeric_limits<T>::min() for signed integer.
        {
            if (min0 < 0 && min0 == std::numeric_limits<result_type>::min()) // TODO: could use offset of 1 if max0 is less than max().
            {
                DFG_ASSERT(false);
                throw std::exception("Unable to initialize UniformIntDistributionImpl object with given values in VC2010 workaround.");
            }
            #ifndef _MSC_VER
                #error Should not get here with current compiler
            #endif
            static_assert(_MSC_VER == 1600, "Build config bug.");
        }
    };

    template <class T2> struct UniformIntTypeWrapper {typedef UniformIntDistributionImpl<T2> type;};
#else
    template <class T2> struct UniformIntTypeWrapper {typedef std::uniform_int_distribution<T2> type;};
#endif // VC2010 buggy std::uniform_int_distribution workaround.

    typedef typename std::conditional<std::is_integral<T>::value,
                                      typename UniformIntTypeWrapper<T>::type,
                                      std::uniform_real_distribution<T> >::type
                                    type;
};

// Returns uniformly distributed value from range min-max.
// For integer cases the range is closed, [min, max] and
// for floating point the inclusion of boundaries is unspecified.
template <class T, class RandEngine>
inline T rand(RandEngine& randEng, const T min, const T max)
//------------------------------------------------------------
{
    // Using int8/uint8 is not supported in VC2013. If T is either one of those, create the value with int
    // and cast it in the end.
    const bool bIntegral = std::is_integral<T>::value;
    typedef typename std::conditional<bIntegral && sizeof(T) == sizeof(char), int, T>::type ImplT;
    typename DFG_CLASS_NAME(DistributionTypeUniform)<ImplT>::type distr(min, max);
    const auto val = distr(randEng);
    DFG_ASSERT_CORRECTNESS(val >= min && val <= max);
    return static_cast<T>(val);
}

#ifdef _MSC_VER // TODO: Less coarse check. This was added due to MinGW version X where rand() doesn't seem to work (returns the same value for consecutive calls).
    // Returns value in half-open range [0, 1)
    // Note: There's no seeding so can be used without initialization.
    // Note: This function is provided for convenience and the implementation can expected to be slow.
    inline double rand()
    //------------------
    {
        std::random_device rd;
        std::uniform_real_distribution<double> urd;
        return urd(rd);
    }
#endif

// Return uniformly distributed value from range 0-1 using given rand engine.
// It's not specified whether boundaries are included
// so do not use this function if the inclusion of boundary values is relevant.
template <class RandEngine>
inline double rand(RandEngine& randEng)
//-------------------------------------
{
    std::uniform_real_distribution<double> distr(0.0, 1.0);
    return distr(randEng);
}

// Returns unseed rand engine.
// Rationale:	Simplifies random engine creation by providing an easy access to
//				random engine that will in most cases fulfill the quality needs.
// Note: In C++11, there is std::default_random_engine.
// Note: Not using std::default_random_engine here because in MinGW GCC 4.8.0 default_random_ending was linear_congruential_engine
// Note: If an unseeded, non-deterministic generator is needed, see std::random_device.
inline std::mt19937 createDefaultRandEngineUnseeded()
{
    return std::mt19937();
}

// Returns randomly seeded rand engine.
inline std::mt19937 createDefaultRandEngineRandomSeeded()
{
    const std::mt19937::result_type seed = std::random_device()();
    return std::mt19937(seed);
}

// Wrapper for engine and distribution.
// Note that this class takes reference to engine.
template <class RandEngine, class Distr>
struct DFG_CLASS_NAME(DistributionEngine)
//=======================================
{
    typedef typename Distr::result_type result_type;
    DFG_CLASS_NAME(DistributionEngine)(const result_type minRange, const result_type maxRange, RandEngine* pNonNullRe) :
        m_distr(minRange, maxRange),
        m_pRandEngine(pNonNullRe) {}
    result_type operator()() {return m_distr(*m_pRandEngine);}

    Distr m_distr;
    RandEngine* m_pRandEngine;
};

// Utility function for creating DistributionEngine with uniform distribution.
template <class RandEngine, class Type>
DFG_CLASS_NAME(DistributionEngine)<RandEngine, typename DFG_CLASS_NAME(DistributionTypeUniform)<Type>::type> makeDistributionEngineUniform(RandEngine* pRe, const Type minRange, const Type maxRange)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
    return DFG_CLASS_NAME(DistributionEngine)<RandEngine, typename DFG_CLASS_NAME(DistributionTypeUniform)<Type>::type>(minRange, maxRange, pRe);
}

#endif // DFG_LANGFEAT_HAVE_CPP11RAND

}} // module rand

#endif // include guard
