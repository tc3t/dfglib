#pragma once

#include "../dfgDefs.hpp"
#include "../build/LanguageFeatureInfo.hpp"
#include "../preprocessor/compilerInfoMsvc.hpp"
#include <type_traits>

#if DFG_LANGFEAT_CHRONO_11
    #include <chrono>
#endif

#if !DFG_LANGFEAT_CHRONO_11 || (defined(_MSC_VER) && _MSC_VER <= DFG_MSVC_VER_2015)
    // steady_clock is not steady in VC2012 and VC2013  (https://connect.microsoft.com/VisualStudio/feedback/details/753063/, https://connect.microsoft.com/VisualStudio/feedback/details/753115/)
    #include <boost/timer.hpp> // TODO(fix): this should be included always when ClockType::is_steady is false.
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(time) {

// Related code: boost::chrono::high_resolution_clock, boost/timer/timer.hpp,  std::chrono::high_resolution_clock.
class DFG_CLASS_NAME(TimerCpu)
{
public:
#if DFG_LANGFEAT_CHRONO_11
    typedef std::chrono::steady_clock ClockType;
    #if (!defined(_MSC_VER) || _MSC_VER >= DFG_MSVC_VER_2015) // See comments above for details about MSVC.
        static const bool s_bHasSteadyChronoClockType = true;
        typedef void NonChronoType;
        typedef ClockType::time_point TimePoint;
    #else
        static const bool s_bHasSteadyChronoClockType = false;
        typedef boost::timer NonChronoType;
        typedef void TimePoint;
    #endif
#else
    static const bool s_bHasSteadyChronoClockType = false;
    typedef boost::timer NonChronoType;
    typedef void TimePoint;
#endif
    typedef std::integral_constant<bool, s_bHasSteadyChronoClockType> ImplementationTag;

	DFG_CLASS_NAME(TimerCpu)()
    {
        privConstruct(*this, ImplementationTag());
    }

    template <class This_T>
    static void privConstruct(This_T& rThis, std::true_type)
    {
        rThis.m_impl = ClockType::now();
    }

    template <class This_T>
    void privConstruct(This_T&, std::false_type)
    {
        // boost::timer starts automatically so nothing to do here.
    }
    
	double elapsedWallSeconds() const
    {
        return privElapsedWallSeconds(*this, ImplementationTag());
    }

    template <class This_T>
    static double privElapsedWallSeconds(This_T& rThis, std::true_type)
    {
        return std::chrono::duration<double>(ClockType::now() - rThis.m_impl).count();
    }

    template <class This_T>
    static double privElapsedWallSeconds(This_T& rThis, std::false_type)
    {
        return rThis.m_impl.elapsed();
    }

public:
    std::conditional<s_bHasSteadyChronoClockType, TimePoint, NonChronoType>::type m_impl;
}; // class TimerCpu

}} // module time
