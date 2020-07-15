#ifdef _WIN32

/*
 * Tools for handling structured exceptions on Windows
 *
 * Further reading:
 *      -"What actions do I need to take to get a crash dump in ALL error scenarios?"
 *          -https://stackoverflow.com/questions/13591334/what-actions-do-i-need-to-take-to-get-a-crash-dump-in-all-error-scenarios
 *      -"Exceptions that are thrown from an application that runs in a 64-bit version of Windows are ignored":
 *          -https://support.microsoft.com/en-us/help/976038/exceptions-that-are-thrown-from-an-application-that-runs-in-a-64-bit-v
 *      -"Is there a function to convert EXCEPTION_POINTERS struct to a string?"
 *          -http://stackoverflow.com/questions/3523716/is-there-a-function-to-convert-exception-pointers-struct-to-a-string
 */

#include "structuredExceptionHandling.h"
#include "../dfgAssert.hpp"
#include "../str/format_fmt.hpp"
#include "../os.hpp"

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <Windows.h>
    #include <DbgHelp.h>
    #include <process.h> // For _getpid()
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

// Adding link directives.
#if defined(_MSC_VER)
    #pragma comment(lib, "User32") // Without this MessageBoxA will cause linker error.
#endif // defined(_MSC_VER)

bool DFG_MODULE_NS(debug)::structuredExceptionHandling::gbAssumeStdExceptionObjectInExceptionRecordParams = true;

namespace
{
    const DFG_ROOT_NS::uint32 gnExceptionCodeCppException = 0xE06D7363;
}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(debug)
{
    namespace DFG_DETAIL_NS
    {
        LONG WINAPI exceptionHandlerCallback(_EXCEPTION_POINTERS* pExceptionInfo);

        static structuredExceptionHandling::UnhandledExceptionHandler gUnhandledExceptionHandler;
    }
} } // Module debug

bool DFG_MODULE_NS(debug)::structuredExceptionHandling::setUnhandledExceptionHandler(UnhandledExceptionHandler handler)
{
    DFG_DETAIL_NS::gUnhandledExceptionHandler = handler;
    auto rv = ::SetUnhandledExceptionFilter(&DFG_DETAIL_NS::exceptionHandlerCallback);
    return (rv != nullptr);
}

// Returns text corresponding a numeric exception code.
// For related discussion, see http://stackoverflow.com/questions/3523716/is-there-a-function-to-convert-exception-pointers-struct-to-a-string
const char* DFG_MODULE_NS(debug)::structuredExceptionHandling::exceptionCodeToStr(const uint32 code)
{
#define DFG_TEMP_HANDLE_CASE(CODE) case CODE: return #CODE;
    switch (code)
    {
        DFG_TEMP_HANDLE_CASE(EXCEPTION_ACCESS_VIOLATION);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_DATATYPE_MISALIGNMENT);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_BREAKPOINT);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_SINGLE_STEP);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_DENORMAL_OPERAND);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_INEXACT_RESULT);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_INVALID_OPERATION);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_OVERFLOW);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_STACK_CHECK);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_FLT_UNDERFLOW);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_INT_DIVIDE_BY_ZERO);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_INT_OVERFLOW);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_PRIV_INSTRUCTION);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_IN_PAGE_ERROR);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_ILLEGAL_INSTRUCTION);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_NONCONTINUABLE_EXCEPTION);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_STACK_OVERFLOW);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_INVALID_DISPOSITION);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_GUARD_PAGE);
        DFG_TEMP_HANDLE_CASE(EXCEPTION_INVALID_HANDLE);
        DFG_TEMP_HANDLE_CASE(CONTROL_C_EXIT);
        case gnExceptionCodeCppException: return "C++ exception"; // Value in decimal is 3765269347. For more details see http://support.microsoft.com/kb/185294, http://blogs.msdn.com/b/oldnewthing/archive/2010/07/30/10044061.aspx
        default: return "Unknown code";
    }
#undef DFG_TEMP_HANDLE_CASE
}

static std::string createExceptionDescription(_EXCEPTION_POINTERS* pExceptionInfo, const DFG_ROOT_NS::DFG_CLASS_NAME(StringViewSzC)& svCrashDumpPath)
{
    using namespace DFG_MODULE_NS(debug)::structuredExceptionHandling;
    std::string sMsg;

    const auto bAssumeStdExceptionObject = gbAssumeStdExceptionObjectInExceptionRecordParams;

    const bool bHasExceptionRecord = (pExceptionInfo && pExceptionInfo->ExceptionRecord);
    const auto nExceptionCode = (bHasExceptionRecord) ? pExceptionInfo->ExceptionRecord->ExceptionCode : 0;

    const bool bIsCppException = (bHasExceptionRecord && nExceptionCode == gnExceptionCodeCppException);
    const auto sExePath = DFG_MODULE_NS(os)::getExecutableFilePath<char>();

    sMsg = DFG_ROOT_NS::format_fmt("{} (PID {}) has encountered a fatal error and will quit now.\n\nException code: 0x{:x} (\"{}\")\nException parameter count: {}\n",
        DFG_MODULE_NS(os)::pathFilename(sExePath.c_str()),
        _getpid(),
        (bHasExceptionRecord) ? nExceptionCode : 0,
        (bHasExceptionRecord) ? exceptionCodeToStr(nExceptionCode) : "",
        (bHasExceptionRecord) ? static_cast<int>(pExceptionInfo->ExceptionRecord->NumberParameters) : -1);

    if (bIsCppException)
    {
        if (pExceptionInfo->ExceptionRecord->NumberParameters >= 2 && bAssumeStdExceptionObject)
        {
            // The exception object seems to be as the second object (see https://blogs.msdn.microsoft.com/oldnewthing/20100730-00/?p=13273/ ("Decoding the parameters of a thrown C++ exception (0xE06D7363)")
            // and implementation of exception throwing in throw.cpp).
            const uintptr_t v1 = pExceptionInfo->ExceptionRecord->ExceptionInformation[1];
            auto pExceptionObject = reinterpret_cast<const std::exception*>(v1);
            sMsg += DFG_ROOT_NS::format_fmt("\nC++ exception description (using experimental exception interpretation):\n\"{}\"\n", pExceptionObject->what());
        }
        else
        {
            sMsg += "\nNo further info on exception is available for the following reason:\n";
            if (!bAssumeStdExceptionObject)
                sMsg += "Experimental exception interpretation is disabled.";
            else
                sMsg += "Exception record has unexpected parameter count.";
            sMsg += "\n";
        }
    }

    // If we have crash dump path, add question about crash dump creation.
    if (!svCrashDumpPath.empty())
        sMsg += DFG_ROOT_NS::format_fmt("\nBefore terminating, application can try to create a crash dump file to path {0}.\n\nCreate the crash dump file?", svCrashDumpPath.c_str());

    return sMsg;
}

LONG WINAPI DFG_MODULE_NS(debug)::DFG_DETAIL_NS::exceptionHandlerCallback(EXCEPTION_POINTERS* pExceptionInfo)
{
    static_assert(std::is_same<PTOP_LEVEL_EXCEPTION_FILTER, decltype(&exceptionHandlerCallback)>::value, "Bad exception filter callback type");
    if (gUnhandledExceptionHandler)
        return gUnhandledExceptionHandler(pExceptionInfo);
    else
        return EXCEPTION_CONTINUE_SEARCH; // Typically results to normal "Application has crashed"-notification from Windows.
}

namespace
{
    static bool showInfoBoxAndConfirmCrashDumpCreation(_EXCEPTION_POINTERS* pExceptionInfo, const std::string& outputPath)
    {
        const auto sDesc = createExceptionDescription(pExceptionInfo, outputPath);
        const auto crashDumpMbRv = ::MessageBoxA(nullptr, sDesc.c_str(), "Application crash", (outputPath.empty()) ? MB_OK : MB_YESNO);
        if (crashDumpMbRv == IDNO)
            return false;
        return true;
    }
} // unnamed namespace

auto DFG_MODULE_NS(debug)::structuredExceptionHandling::createCrashDump(_EXCEPTION_POINTERS* pExceptionInfo, const std::string& outputPath) -> CreateCrashDumpReturnValue
{
    if (outputPath.empty())
        return CreateCrashDumpReturnValue_noOutputPath;

    HMODULE modDebugHelp = ::LoadLibraryW(L"dbghelp.dll");
    if (!modDebugHelp)
        return CreateCrashDumpReturnValue_failedToLoadDebugHlp;

    typedef decltype(&MiniDumpWriteDump) MiniDumpWriterFunctionPointer;

    auto fpMiniDumpWriteDump = reinterpret_cast<MiniDumpWriterFunctionPointer>(::GetProcAddress(modDebugHelp, "MiniDumpWriteDump"));

    if (!fpMiniDumpWriteDump)
        return CreateCrashDumpReturnValue_didNotFindDumpWriterProg;

    HANDLE hFile = ::CreateFileA(outputPath.c_str(), GENERIC_WRITE, 0 /*no sharing*/, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return CreateCrashDumpReturnValue_failedToCreateOutputFile;

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;

    exceptionInfo.ThreadId = ::GetCurrentThreadId();
    // Information about thread names: see http://stackoverflow.com/questions/9366722/how-to-get-the-name-of-a-win32-thread
    exceptionInfo.ExceptionPointers = pExceptionInfo;
    exceptionInfo.ClientPointers = FALSE;

    // Note: Documentation of MiniDumpWriteDump notes: "MiniDumpWriteDump should be called from a separate process if at all possible, rather than from within the target process being dumped."
    const BOOL bMdWriter = fpMiniDumpWriteDump(GetCurrentProcess(),
                                             GetCurrentProcessId(),
                                             hFile,
                                             MiniDumpNormal,
                                             &exceptionInfo,
                                             NULL,
                                             NULL);
    ::CloseHandle(hFile);

    if (bMdWriter != 0)
        return CreateCrashDumpReturnValue_success;
    else // case: failed to write minidump.
        return CreateCrashDumpReturnValue_failedToWriteMinidump;
}

namespace
{
    static char gszDumpPath[_MAX_PATH] = ""; // Receive space for dump path to avoid memory allocation.
}

long DFG_MODULE_NS(debug)::structuredExceptionHandling::unhandledExceptionHandler_miniDumpCreator(_EXCEPTION_POINTERS* pExceptionInfo)
{
    if (!showInfoBoxAndConfirmCrashDumpCreation(pExceptionInfo, gszDumpPath))
        return EXCEPTION_CONTINUE_SEARCH; // Typically results to normal "Application has crashed"-notification from Windows.

    const auto rv = createCrashDump(pExceptionInfo, gszDumpPath);
    if (rv == CreateCrashDumpReturnValue_success)
    {
        auto sMsg = format_fmt("A crash dump was successfully created to path '{}'", gszDumpPath);
        ::MessageBoxA(NULL, sMsg.c_str(), "Crash dump created", MB_OK);
        return EXCEPTION_EXECUTE_HANDLER; // "This usually results in process termination."
    }
    else
    {
        auto sMsg = format_fmt("Failed to create crash dump, error code {}", rv);
        ::MessageBoxA(NULL, sMsg.c_str(), "Crash dump creation failed", MB_OK);
        return EXCEPTION_CONTINUE_SEARCH; // Typically results to normal "Application has crashed"-notification from Windows.
    }
}

void DFG_MODULE_NS(debug)::structuredExceptionHandling::enableAutoDumps(const char* pszDumpPath)
{
    if (!pszDumpPath)
    {
        DFG_ASSERT_INVALID_ARGUMENT(pszDumpPath != nullptr, "Expecting valid path");
        return;
    }
    strcpy_s(gszDumpPath, pszDumpPath);
    setUnhandledExceptionHandler(&unhandledExceptionHandler_miniDumpCreator);
}

#endif // _WIN32
