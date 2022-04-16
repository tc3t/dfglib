// Helper interface to build format.cc as there has been some
// problems with using .cc-file directly in project files
//     https://lists.qt-project.org/pipermail/qt-creator/2017-July/006661.html ("qmake will not build c++ file named something like format.cc and ostream.cc")
//     https://bugreports.qt.io/browse/QTBUG-61842

#include "../../dfgDefs.hpp"

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include "format.cc"
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
