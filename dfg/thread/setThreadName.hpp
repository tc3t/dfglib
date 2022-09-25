#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(thread) {

//https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
#ifdef _MSC_VER
    // Note: Thread name is only for debugger visualization, there's no name in thread objects. (http://stackoverflow.com/questions/9366722/how-to-get-the-name-of-a-win32-thread)
    void setThreadName(const char* pszThreadName);
    void setThreadName(unsigned long threadId, const char* pszThreadName);
#else
    // setThreadName() is not available in current environment.
#endif

} } // module namespace
