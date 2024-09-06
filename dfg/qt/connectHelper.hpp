#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"

#ifdef DFG_BUILD_TYPE_DEBUG
    #define DFG_QT_VERIFY_CONNECT(x) DFG_VERIFY_WITH_MSG(static_cast<bool>(x), "Qt connect failed."); // Without cast caused warning C4552 ("operator has no effect") in MSVC release builds.
#else
    #define DFG_QT_VERIFY_CONNECT(x) x
#endif
