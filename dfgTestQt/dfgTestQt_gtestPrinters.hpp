#pragma once

class QString;

#include <iosfwd>

// GoogleTest printer for QString, for details about printers, see comments in dfgTest.hpp
void PrintTo(const QString& s, ::std::ostream* pOstrm);
