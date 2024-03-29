#pragma once

#include "../dfgDefs.hpp"
#include <optional>

#if _WIN32
    #include <Windows.h>
    #include <psapi.h>
#endif // _WIN32

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

    namespace DFG_DETAIL_NS
    {
        class ProcessMemoryInfoBase
        {
        public:
            using ByteCountOpt = std::optional<size_t>;

            static constexpr ByteCountOpt workingSetSize()      { return ByteCountOpt(); } // Current working set size, in bytes.
            static constexpr ByteCountOpt workingSetPeakSize()  { return ByteCountOpt(); } // Peak working set size, in bytes.
            static constexpr ByteCountOpt pageFaultCount()      { return ByteCountOpt(); } // Number of page faults.

        }; // class ProcessMemoryInfoBase
    } // namespace DFG_DETAIL_NS

#if _WIN32
    class ProcessMemoryInfo : public DFG_DETAIL_NS::ProcessMemoryInfoBase
    {
    public:
        ProcessMemoryInfo();

        ByteCountOpt workingSetSize()        const { return (m_counters.has_value() ? m_counters.value().WorkingSetSize       : ByteCountOpt{}); }
        ByteCountOpt workingSetPeakSize()    const { return (m_counters.has_value() ? m_counters.value().PeakWorkingSetSize   : ByteCountOpt{}); }
        ByteCountOpt pageFaultCount()        const { return (m_counters.has_value() ? m_counters.value().PageFaultCount       : ByteCountOpt{}); }

        std::optional<PROCESS_MEMORY_COUNTERS> m_counters;
    }; // class ProcessMemoryInfo

    inline ProcessMemoryInfo::ProcessMemoryInfo()
    {
        PROCESS_MEMORY_COUNTERS counters;
        if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            &counters,
            sizeof(counters)
            ) != 0)
        {
            m_counters = counters;
        }
    }

#else // Case: other than Windows, not implemented so using default empty implementation
    using ProcessMemoryInfo = DFG_DETAIL_NS::ProcessMemoryInfoBase;
#endif // _WIN32

// Returns memory info snapshot for current process
//      -Windows: provides all details available in ProcessMemoryInfo
//      -Others: not implemented (all functions will returns nullopt)
// @note To format byte counts for easier readability, see dfg/str/byteCountFormatter.hpp
inline ProcessMemoryInfo getMemoryUsage_process()
{
    return ProcessMemoryInfo{};
}

} } // namespace dfg::os
