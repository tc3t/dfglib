#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"

#define DFG_QT_VERIFY_CONNECT(x) DFG_ASSERT_WITH_MSG(static_cast<bool>(x), "Qt connect failed.");
