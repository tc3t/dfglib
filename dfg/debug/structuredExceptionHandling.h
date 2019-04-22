#pragma once

#ifdef _WIN32

#include "../dfgDefs.hpp"
#include "../dfgBaseTypedefs.hpp"
#include <functional>
#include <string>

struct _EXCEPTION_POINTERS;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(debug)
{
    namespace structuredExceptionHandling
    {
        enum CreateCrashDumpReturnValue
        {
            CreateCrashDumpReturnValue_success,
            CreateCrashDumpReturnValue_failedToLoadDebugHlp,
            CreateCrashDumpReturnValue_didNotFindDumpWriterProg,
            CreateCrashDumpReturnValue_failedToWriteMinidump,
            CreateCrashDumpReturnValue_failedToCreateOutputFile,
            CreateCrashDumpReturnValue_noOutputPath
        };

        typedef std::function<long(_EXCEPTION_POINTERS* pExceptionInfo)> UnhandledExceptionHandler;

        /* Sets unhandled exception handler
         * Example:
         * dfg::debug::structuredExceptionHandling::setUnhandledExceptionHandler([](_EXCEPTION_POINTERS* pExceptionInfo) -> long
                                {
                                    dfg::debug::structuredExceptionHandling::createCrashDump(pExceptionInfo, "c:/users/user/Desktop/crashDumpTesti.dmp");
                                    return EXCEPTION_EXECUTE_HANDLER;
                                });
         * Returns true if handler was successfully set, false otherwise.
        */
        bool setUnhandledExceptionHandler(UnhandledExceptionHandler handler);

        // Creates crash dump file from given exceptionPointers to given path.
        CreateCrashDumpReturnValue createCrashDump(_EXCEPTION_POINTERS* pExceptionInfo, const std::string& outputPath);

        // Returns text representation of pExceptionInfo->ExceptionRecord->ExceptionCode
        // Returned string is NOT owned by caller and has static lifetime.
        const char* exceptionCodeToStr(const uint32 code);

        // Enables creation of dumps in case of crash. Dumps are stored to given file path.
        void enableAutoDumps(const char* pszDumpPath);

        long unhandledExceptionHandler_miniDumpCreator(_EXCEPTION_POINTERS* pExceptionInfo);

        extern bool gbAssumeStdExceptionObjectInExceptionRecordParams;
    } // namespace structuredExceptionHandling

}} // module namespace

#endif // _WIN32
