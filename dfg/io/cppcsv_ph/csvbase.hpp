#pragma once

// for size_t
#include <cstddef>

namespace cppcsv {


   struct per_cell_tag {};
   struct per_row_tag {};

   /* Discouraging virtual interface to encourage template-based speed.
    * Library users can create their own virtual base class if required.
    *
// basic virtual interface for catching the begin/cell/end emissions.
// You can avoid the virtual calls by templating csv_parser on your own builder.
template <class Char>
class csv_builder_t : public per_cell_tag { // abstract base
public:
  virtual ~csv_builder_t() {}

  virtual void begin_row() = 0;
  virtual void cell( const Char *buf, size_t len ) = 0;  // buf can be NULL
  virtual void end_row() = 0;
};

typedef csv_builder_t<char> csv_builder;
*/


// This adaptor allows you to use csv_parser on a emit-per-row basis
// The function attached will be called once per row.
// This is NOT designed to be inherited from.
//
// Note, the cell() and end_full_row() accepts non-const buffer :)

template <class Function>
class csv_builder_bulk : public per_row_tag {
public:
  Function function;

  csv_builder_bulk(Function func) : function(func) {}

  // this calls the attached function
  // note that there is (num_cells+1) offsets,
  // the last offset is the one-past-the-end index of the last cell
  void end_full_row(
        char* buffer,
        size_t num_cells,
        const size_t * offsets, // array[num_cells+1]
        size_t file_row          // row where these cells started
        )
  {
    function(buffer, num_cells, offsets, file_row);
  }
};

}
