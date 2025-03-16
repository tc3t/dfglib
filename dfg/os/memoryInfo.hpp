#pragma once

#include "../dfgDefs.hpp"
#include <optional>


#if defined(_WIN32)
    #include <Windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include "../cont/MapVector.hpp"
    #include "../io/BasicIfStream.hpp"
    #include "../str/strTo.hpp"
    #include <string>
#endif // Platform


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os) {

    /*
    Related links:
        -https://stackoverflow.com/questions/48417499/programmatically-retrieve-peak-virtual-memory-of-a-process
        -https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    */
    namespace DFG_DETAIL_NS
    {
        class SystemMemoryInfoBase
        {
        public:
            using ByteCountOpt = std::optional<uint64>;

            // Returns the total non-virtual memory in system (in bytes) or empty optional if not available.
            static constexpr ByteCountOpt total()                    { return ByteCountOpt{}; }
            // Returns available memory in bytes or empty optional if not available.
            // Exact meaning of "available" may be somewhat platforms-specific.
            static constexpr ByteCountOpt available()                { return ByteCountOpt{}; }
            // Returns used memory percentage as int 0-100 (rounding is unspecified) or empty optional if not available.
            static constexpr ByteCountOpt usedPercentageInt()        { return ByteCountOpt{}; }
        }; // class SystemMemoryInfoBase

        class ProcessMemoryInfoBase
        {
        public:
            using ByteCountOpt = std::optional<size_t>;

            static constexpr ByteCountOpt workingSetSize()      { return ByteCountOpt(); } // Current working set size, in bytes.
            static constexpr ByteCountOpt workingSetPeakSize()  { return ByteCountOpt(); } // Peak working set size, in bytes.
            static constexpr ByteCountOpt pageFaultCount()      { return ByteCountOpt(); } // Number of page faults.
            static constexpr ByteCountOpt virtualMemoryPeak()   { return ByteCountOpt(); } // Peak virtual memory usage

        }; // class ProcessMemoryInfoBase
    } // namespace DFG_DETAIL_NS

#if defined(_WIN32)

    class SystemMemoryInfo : public DFG_DETAIL_NS::SystemMemoryInfoBase
    {
    public:
        SystemMemoryInfo();

        ByteCountOpt total()                    const { return (m_memoryInfo.has_value() ? m_memoryInfo.value().ullTotalPhys : ByteCountOpt{}); }
        ByteCountOpt available()                const { return (m_memoryInfo.has_value() ? m_memoryInfo.value().ullAvailPhys : ByteCountOpt{}); }
        ByteCountOpt usedPercentageInt()        const { return (m_memoryInfo.has_value() ? m_memoryInfo.value().dwMemoryLoad : ByteCountOpt{}); }

        // Related WinApi functionality: GetPerformanceInfo() / PERFORMANCE_INFORMATION
        std::optional<MEMORYSTATUSEX> m_memoryInfo;
        DWORD m_nLastError = ERROR_SUCCESS; // If retrieving memory information fails, GetLastError() is stored here. 
    }; // class SystemMemoryInfo

    inline SystemMemoryInfo::SystemMemoryInfo()
    {
        m_memoryInfo = MEMORYSTATUSEX();
        auto& memInfo = m_memoryInfo.value();
        ZeroMemory(&memInfo, sizeof(memInfo));
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo))
            return;
        else
        {
            m_nLastError = GetLastError();
            m_memoryInfo.reset();
            return;
        }
    }

    class ProcessMemoryInfo : public DFG_DETAIL_NS::ProcessMemoryInfoBase
    {
    public:
        ProcessMemoryInfo();

        ByteCountOpt workingSetSize()        const { return (m_counters.has_value() ? m_counters.value().WorkingSetSize       : ByteCountOpt{}); }
        ByteCountOpt workingSetPeakSize()    const { return (m_counters.has_value() ? m_counters.value().PeakWorkingSetSize   : ByteCountOpt{}); }
        ByteCountOpt pageFaultCount()        const { return (m_counters.has_value() ? m_counters.value().PageFaultCount       : ByteCountOpt{}); }
        ByteCountOpt virtualMemoryPeak()     const { return (m_counters.has_value() ? m_counters.value().PeakPagefileUsage    : ByteCountOpt{}); }

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
#elif defined(__linux__)

    using SystemMemoryInfo = DFG_DETAIL_NS::SystemMemoryInfoBase;

    class ProcessMemoryInfo : public DFG_DETAIL_NS::ProcessMemoryInfoBase
    {
    public:
        ProcessMemoryInfo();

        ByteCountOpt getField(const StringViewC svKey) const
        {
            return m_fields.valueCopyOr(svKey, ByteCountOpt());
        }

        ByteCountOpt virtualMemoryPeak()     const { return getField("VmPeak"); }

        ::DFG_MODULE_NS(cont)::MapVectorSoA<std::string, ByteCountOpt> m_fields;
    }; // class ProcessMemoryInfo

    inline ProcessMemoryInfo::ProcessMemoryInfo()
    {
        // Reading /proc/self/status. Note that documentation ('man proc') in Ubuntu 22.04 has comment:
        //      > This value is inaccurate
        // for fields like VmHWM and VmRSS and also that
        //      > Some of these values are inaccurate because of a kernel-internal scalability optimization.  If accurate
        //      > values are required, use /proc/[pid]/smaps or /proc/[pid]/smaps_rollup instead, which are much slower
        //      > but provide accurate, detailed information
        
        ::DFG_MODULE_NS(io)::BasicIfStream istrm("/proc/self/status");
        std::string s;
        for (int c = istrm.get(); c != istrm.eofVal(); c = istrm.get())
        {
            if (c != '\n')
                s.push_back(static_cast<char>(c));
            else
            {
                // Storing fields that begin with Vm or Rss
                const char* p = s.c_str();
                if (s.size() >= 2 && s[0] == 'V' && s[1] == 'm')
                    p += 2;
                else if (s.size() >= 3 && s[0] == 'R' && s[1] == 's' && s[2] == 's')
                    p += 3;
                else
                    p = nullptr;

                if (p)
                {
                    using namespace ::DFG_MODULE_NS(str);
                    // Example line: "VmPeak:     4032 kB"
                    // Skipping to one past field name
                    while (*p != '\0' && *p != ':')
                        ++p;
                    const char* const pColon = p;
                    // Skipping whitespaces before number
                    while (*p != '\0' && !std::isdigit(*p))
                        ++p;
                    const char* pStartOfDigits = p;
                    // Skipping digits
                    while (std::isdigit(*p))
                        ++p;
                    bool bOk = false;
                    const auto val = strTo<size_t>(StringViewC(pStartOfDigits, p), &bOk);
                    if (bOk)
                    {
                        // Skipping spaces between number and unit
                        while (*p != '\0' && (*p == ' ' || *p == '\t'))
                            ++p;
                        size_t nFactor = 0;
                        if (*p == 'k') // Should apparently be always true, https://unix.stackexchange.com/questions/199482/does-proc-pid-status-always-use-kb
                            nFactor = 1024;
                        if (nFactor > 0)
                            m_fields.insert(std::string(s.c_str(), pColon), val * nFactor);
                    }

                }
                s.clear();
            }
        }
    }
#else // Case: other than Windows and Linux, not implemented so using default empty implementation
    using SystemMemoryInfo = DFG_DETAIL_NS::SystemMemoryInfoBase;
    using ProcessMemoryInfo = DFG_DETAIL_NS::ProcessMemoryInfoBase;
#endif // Platform

// Returns memory info snapshot for current process
//      -Windows: provides all details available in ProcessMemoryInfo
//      -Linux: only virtualMemoryPeak() is implemented
//      -Others: not implemented (all functions return nullopt)
// @note To format byte counts for easier readability, see dfg/str/byteCountFormatter.hpp
inline ProcessMemoryInfo getMemoryUsage_process()
{
    return ProcessMemoryInfo{};
}

// Returns memory info snapshot for system
//      -Windows: provides all details available in SystemMemoryInfo
//      -Others: none of the fields are implemented (all functions return nullopt)
// @note To format byte counts for easier readability, see dfg/str/byteCountFormatter.hpp
inline SystemMemoryInfo getMemoryUsage_system()
{
    return SystemMemoryInfo{};
}

} } // namespace dfg::os
