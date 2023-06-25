#pragma once

class QModelIndex;
class QString;

#include <iosfwd>

// GoogleTest printers for some Qt types, for details about printers, see comments in dfgTest.hpp
void PrintTo(const QString& s, ::std::ostream* pOstrm);
void PrintTo(const QModelIndex& i, ::std::ostream* pOstrm);
