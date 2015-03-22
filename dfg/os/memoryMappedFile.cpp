#include "memoryMappedFile.hpp"

// FIXME: This file is expected to cause link errors if using this in a project that links with boost.

/*
Note: If MinGW compilation causes error such as
"error : there are no arguments to '_assert' that depend on a template parameter, so a declaration of '_assert' must be available"
the remedy may be to define macro NDEBUG in build.
*/

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include "boost/mapped_file.cpp"
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

