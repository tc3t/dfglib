#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"

#ifdef _DEBUG
    #define DFG_QT_VERIFY_CONNECT(x) DFG_VERIFY_WITH_MSG(static_cast<bool>(x), "Qt connect failed."); // This causes warning C4552 ("operator has no effect") in release builds.
#else
    #define DFG_QT_VERIFY_CONNECT(x) x
#endif
