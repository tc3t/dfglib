#pragma once

// Copyright (c) 2013 Tobias Hoffmann
//               2013 Paul Harris <paulharris@computer.org>
//
// License: http://opensource.org/licenses/MIT

#include <cassert>
#include <cstring>
#include "csvbase.hpp"

#include <algorithm>
#include <string>
#include <vector>

#define CPPCSV_DEBUG 0

#if CPPCSV_DEBUG
  #include <cstdio>
  #include <typeinfo>
#endif

namespace cppcsv {
namespace csvFSM {

   // State Indexes (in the transition array)
   enum StateIdx {
      Start = 0,
      ReadSkipPre,
      ReadQuoted,
      ReadQuotedCheckEscape,
      ReadQuotedSkipPost,
      ReadDosCR,
      ReadQuotedDosCR,
      ReadUnquoted,
      ReadUnquotedWhitespace,
      ReadComment,
      ReadError,
      NUM_SI
   };

   // State Transitions
   template <class TTrans>
   class ST_Base {
   public:
      virtual ~ST_Base() {} // to avoid the abstract class delete warning

      virtual StateIdx Echar( TTrans& t ) const = 0;
      virtual StateIdx Ewhitespace( TTrans& t ) const = 0;
      virtual StateIdx Eqchar( TTrans& t ) const = 0;
      virtual StateIdx Esep( TTrans& t ) const = 0;
      virtual StateIdx Enewline( TTrans& t ) const = 0;
      virtual StateIdx Edos_cr( TTrans& t ) const = 0;   // DOS carriage return
      virtual StateIdx Ecomment( TTrans& t ) const = 0;
   };




template <class Output>
void call_out_begin_row( Output & out, per_cell_tag & )
{
   out.begin_row();
}

template <class Output>
void call_out_cell( Output & out, per_cell_tag & )
{
   out.cell(NULL, 0);
}

template <class Output, typename Char>
void call_out_cell( Output & out, per_cell_tag &, const Char *buf, size_t len )
{
   out.cell(buf,len);
}


template <class Output, typename Char>
void call_out_end_full_row(
      Output & out, per_cell_tag &,
      Char* buffer,
      size_t num_cells,
      const size_t * offsets,
      size_t file_row
      )
{
   // just calls end_row()
   out.end_row();
}



// and now for the per-row builders
template <class Output>
void call_out_begin_row( Output & out, per_row_tag & )
{
   // do nothing
}

template <class Output, typename Char>
void call_out_cell( Output & out, per_row_tag &, const Char *buf, size_t len )
{
   // do nothing
}


template <class Output>
void call_out_cell( Output & out, per_row_tag & )
{
   // do nothing
}


template <class Output>
void call_out_end_row( Output & out, per_row_tag & )
{
   // do nothing
}

template <class Output, typename Char>
void call_out_end_full_row(
      Output & out, per_row_tag &,
      Char* buffer,
      size_t num_cells,
      const size_t * offsets,
      size_t file_row
      )
{
   out.end_full_row(buffer, num_cells, offsets, file_row);
}




template <class CsvBuilder>
class Trans
{
   // disable copy
   Trans& operator=(Trans const&);
   Trans(Trans const&);

public:
  Trans(CsvBuilder &out, bool trim_whitespace, bool collapse_separators ) :
     value(0),
     row_file_start_row(0),
     active_qchar(0),
     error_message(NULL),
     out(out),
     trim_whitespace(trim_whitespace),
     collapse_separators(collapse_separators)
   {
      // Note that I'm using vector<> instead of string
      // because we don't need the extra string abilities,
      // and I found it slow on GCC 4.8
      //
      // some initial buffer size
      cells_buffer.resize(128);
      whitespace_state.resize(32);

      cells_buffer[0] = 0;
      whitespace_state[0] = 0;
      cells_buffer_len = 0;
      whitespace_state_len = 0;
   }

  // this is set before the state change is called,
  // that way the Events do not need to carry their state with them.
  char value;

  size_t row_file_start_row;      // first file row for this row

  // active quote character, required in the situation where multiple quote chars are possible
  // if =0 then there is no active_qchar
  char active_qchar;

public:
  bool row_empty() const {
     // return cells_buffer.empty() && whitespace_state.empty();
     return cells_buffer_len == 0 && whitespace_state_len == 0;
  }

  size_t last_cell_length() const {
     assert(is_row_open());
     return cells_buffer_len - cell_offsets.back();
  }

  bool is_row_open() const {
     return !cell_offsets.empty();
  }

  const char* error_message;


// make public for virtual to call...
// private:

  // adds current character to cell buffer
  void add()
  {
     // cells_buffer.push_back(value);
     if (cells_buffer_len+1 >= cells_buffer.size())
        cells_buffer.resize( cells_buffer.size()*2 );
     cells_buffer[cells_buffer_len] = value;
     ++cells_buffer_len;
     // note: don't bother to null-terminate
     // cells_buffer[cells_buffer_len] = 0;
  }

  // whitespace is remembered until we know if we need to add to the output or forget
  void remember_whitespace()
  {
     // whitespace_state.push_back(value);
     if (whitespace_state_len+1 >= whitespace_state.size())
        whitespace_state.resize( whitespace_state.size()*2 );
     whitespace_state[whitespace_state_len] = value;
     ++whitespace_state_len;
     // note: don't bother to null-terminate whitespace
  }

  // adds remembered whitespace to cell buffer
  void add_whitespace()
  {
     if (whitespace_state_len > 0)
     {
        if (cells_buffer_len+whitespace_state_len >= cells_buffer.size())
           cells_buffer.resize( (cells_buffer.size()+whitespace_state_len)*2 );
        memcpy(&cells_buffer[cells_buffer_len], &whitespace_state[0], whitespace_state_len);
        cells_buffer_len += whitespace_state_len;
        // cells_buffer.append(whitespace_state);
        drop_whitespace();
     }
  }

  // only writes out whitespace IF we aren't trimming
  void add_unquoted_post_whitespace()
  {
     if (whitespace_state_len > 0)
     {
        if (!trim_whitespace)
        {
           if (cells_buffer_len+whitespace_state_len >= cells_buffer.size())
              cells_buffer.resize( (cells_buffer.size()+whitespace_state_len)*2 );
           // cells_buffer.append(whitespace_state);
           memcpy(&cells_buffer[cells_buffer_len], &whitespace_state[0], whitespace_state_len);
           cells_buffer_len += whitespace_state_len;
           // note: don't bother to null-terminate
           // cells_buffer[cells_buffer_len] = 0;
        }
        drop_whitespace();
     }
  }

  void drop_whitespace()
  {
     whitespace_state_len = 0;
     // note: don't bother to null-terminate
  }


  void begin_row()
  {
     assert(!is_row_open());
     cell_offsets.push_back(0);
     call_out_begin_row( out, out );
  }


  // adds an entry to the offsets set
  void next_cell(bool has_content = true)
  {
    assert(is_row_open());
    if (!is_row_open()) throw int(2);
    const size_t start_off = cell_offsets.back();
    // const size_t end_off = cells_buffer.size();
    const size_t end_off = cells_buffer_len;

    cell_offsets.push_back(end_off);

    if (has_content) {
      // assert(!cells_buffer.empty());
      assert(cells_buffer_len != 0);
      // char* base = &cells_buffer[0];
      char* base = &cells_buffer[0];
      call_out_cell( out, out, base+start_off, end_off-start_off );
    } else {
      assert(start_off == end_off);
      assert(last_cell_length() == 0);
      call_out_cell(out, out);
    }
  }

  void end_row()
  {
     assert(is_row_open());

     // give client a NON-CONST buffer
     // so they can modify in-place for better efficiency
     // char * buffer = (cells_buffer.empty() ? NULL : &cells_buffer[0]);
     char * buffer = &cells_buffer[0];

     call_out_end_full_row(
           out, out,
           buffer,                  // buffer
           cell_offsets.size()-1,   // num cells
           &cell_offsets[0],        // offsets
           row_file_start_row       // first file row for this row
           );

     cell_offsets.clear();
     // cells_buffer.clear();
     cells_buffer_len = 0;
     // note: don't bother to null-terminate
  }

  CsvBuilder &out;

  std::vector<char> cells_buffer;
  std::vector<char> whitespace_state;
  unsigned int cells_buffer_len;
  unsigned int whitespace_state_len;

  // std::string cells_buffer;
  // std::string whitespace_state;
  std::vector<size_t> cell_offsets;

  bool trim_whitespace;
  bool collapse_separators;
};



  // for defining a state transition with code
#define TTS(State, Event, TargetState, code) virtual StateIdx Event(TTrans & t) const { code; return TargetState; }

  // for defining a transition that redirects to another Event handler for this state
#define REDIRECT(State, Event, OtherEvent) virtual StateIdx Event(TTrans & t) const { return this->OtherEvent(t); }

template <class TTrans>
class ST_Start : public ST_Base<TTrans> {
public:
  //  State          Event        Next_State    Transition_Action
  TTS(Start,         Eqchar,      ReadQuoted,   { t.active_qchar = t.value; t.begin_row(); });
  TTS(Start,         Esep,        ReadSkipPre,  { t.begin_row(); t.next_cell(false); });
  TTS(Start,         Enewline,    Start,        { t.begin_row(); t.end_row(); });
  TTS(Start,         Edos_cr,     ReadDosCR,   {});
  TTS(Start,         Ewhitespace, ReadSkipPre,  { if (!t.trim_whitespace) { t.remember_whitespace(); } t.begin_row(); });   // we MAY want to remember this whitespace
  TTS(Start,         Echar,       ReadUnquoted, { t.begin_row(); t.add(); });
  TTS(Start,         Ecomment,    ReadComment,  {});  // comment at the start of the line --> everything discarded, no blank row is generated
};



template <class TTrans>
class ST_ReadSkipPre : public ST_Base<TTrans> {
public:
  TTS(ReadSkipPre,   Eqchar,      ReadQuoted,   { t.active_qchar = t.value; t.drop_whitespace(); });   // we always want to forget whitespace before the quotes
  TTS(ReadSkipPre,   Esep,        ReadSkipPre,  { if (!t.collapse_separators) { t.next_cell(false); } });
  TTS(ReadSkipPre,   Enewline,    Start,        { t.next_cell(false); t.end_row(); });
  TTS(ReadSkipPre,   Edos_cr,     ReadDosCR,    { t.next_cell(false); });  // same as newline, except we expect to see newline next
  TTS(ReadSkipPre,   Ewhitespace, ReadSkipPre,  { if (!t.trim_whitespace) { t.remember_whitespace(); } });  // we MAY want to remember this whitespace
  TTS(ReadSkipPre,   Echar,       ReadUnquoted, { t.add_whitespace(); t.add(); });   // we add whitespace IF any was recorded
  TTS(ReadSkipPre,   Ecomment,    ReadComment,  { t.add_whitespace(); if (t.last_cell_length() != 0) { t.next_cell(true); } t.end_row(); } );   // IF there was anything, then emit a cell else completely ignore the current cell (ie do not emit a null)

};



template <class TTrans>
class ST_ReadQuoted : public ST_Base<TTrans> {
public:
  TTS(ReadQuoted,    Eqchar,      ReadQuotedCheckEscape, {});
  TTS(ReadQuoted,    Edos_cr,     ReadQuotedDosCR,       {});  // do not add, we translate \r\n to \n, even within quotes
  TTS(ReadQuoted,    Echar,       ReadQuoted,            { t.add(); });
  REDIRECT(ReadQuoted,    Esep,        Echar )
  REDIRECT(ReadQuoted,    Enewline,    Echar )
  REDIRECT(ReadQuoted,    Ewhitespace, Echar )
  REDIRECT(ReadQuoted,    Ecomment,    Echar )
};



template <class TTrans>
class ST_ReadQuotedCheckEscape : public ST_Base<TTrans> {
public:
  // we are reading quoted text, we see a "... here we are looking to see if its followed by another "
  // if so, then output a quote, else its the end of the quoted section.
  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted,          { t.add(); });
  TTS(ReadQuotedCheckEscape, Esep,        ReadSkipPre,         { t.active_qchar = 0; t.next_cell(); });
  TTS(ReadQuotedCheckEscape, Enewline,    Start,               { t.active_qchar = 0; t.next_cell(); t.end_row(); });
  TTS(ReadQuotedCheckEscape, Edos_cr,     ReadDosCR,           { t.active_qchar = 0; t.next_cell(); });
  TTS(ReadQuotedCheckEscape, Ewhitespace, ReadQuotedSkipPost,  { t.active_qchar = 0; });
  TTS(ReadQuotedCheckEscape, Echar,       ReadError,           { t.error_message = "char after possible endquote"; });
  TTS(ReadQuotedCheckEscape, Ecomment,    ReadComment,         { t.active_qchar = 0; t.next_cell(); t.end_row(); });
};



template <class TTrans>
class ST_ReadQuotedSkipPost : public ST_Base<TTrans> {
public:
  // we have finished a quoted cell, we are just looking for the next separator or end point so we can emit the cell
  TTS(ReadQuotedSkipPost, Eqchar,      ReadError,           { t.error_message = "quote after endquote"; });
  TTS(ReadQuotedSkipPost, Esep,        ReadSkipPre,         { t.next_cell(); });
  TTS(ReadQuotedSkipPost, Enewline,    Start,               { t.next_cell(); t.end_row(); });
  TTS(ReadQuotedSkipPost, Edos_cr,     ReadDosCR,           { t.next_cell(); });
  TTS(ReadQuotedSkipPost, Ewhitespace, ReadQuotedSkipPost,  {});
  TTS(ReadQuotedSkipPost, Echar,       ReadError,           { t.error_message = "char after endquote"; });
  TTS(ReadQuotedSkipPost, Ecomment,    ReadComment,         { t.next_cell(); t.end_row(); });
};



template <class TTrans>
class ST_ReadDosCR : public ST_Base<TTrans> {
public:
  TTS(ReadDosCR,    Eqchar,    ReadError, { t.error_message = "quote after CR"; });
  TTS(ReadDosCR,    Esep,      ReadError, { t.error_message = "sep after CR"; });
  // If row is blank, begin and then end it.
  TTS(ReadDosCR,    Enewline,  Start,     { if (!t.is_row_open()) { t.begin_row(); } t.end_row(); });
  TTS(ReadDosCR,    Edos_cr,   ReadError, { t.error_message = "CR after CR"; });
  TTS(ReadDosCR,    Ewhitespace, ReadError, { t.error_message = "whitespace after CR"; });
  TTS(ReadDosCR,    Echar,     ReadError, { t.error_message = "char after CR"; });
  TTS(ReadDosCR,    Ecomment,  ReadError, { t.error_message = "comment after CR"; });
};



template <class TTrans>
class ST_ReadQuotedDosCR : public ST_Base<TTrans> {
public:
  TTS(ReadQuotedDosCR,    Eqchar,    ReadError,    { t.error_message = "quote after CR"; });
  TTS(ReadQuotedDosCR,    Esep,      ReadError,    { t.error_message = "sep after CR"; });
  TTS(ReadQuotedDosCR,    Enewline,  ReadQuoted,   { t.add(); });   // we see \r\n, so add(\n) and continue reading Quoted
  TTS(ReadQuotedDosCR,    Edos_cr,   ReadError,    { t.error_message = "CR after CR"; });
  TTS(ReadQuotedDosCR,    Ewhitespace, ReadError,  { t.error_message = "whitespace after CR"; });
  TTS(ReadQuotedDosCR,    Echar,     ReadError,    { t.error_message = "char after CR"; });
  TTS(ReadQuotedDosCR,    Ecomment,  ReadError,    { t.error_message = "comment after CR"; });
};



template <class TTrans>
class ST_ReadUnquoted : public ST_Base<TTrans> {
public:
  // we have seen some text, and we are reading it in
  // TTS(ReadUnquoted, Eqchar,      ReadError,              { error_message = "unexpected quote in unquoted string"; });
  // Modified, if we see a quote in an unquoted cell, then just read as a plain quote
  // The rule of always quoting cells with quotes is a "please do" but Excel does not practice it when writing to Clipboard, and is tolerant when reading.
  // So Eqchar is handled in the same way as Echar (below)

  TTS(ReadUnquoted, Esep,        ReadSkipPre,            { t.next_cell(); });
  TTS(ReadUnquoted, Enewline,    Start,                  { t.next_cell(); t.end_row(); });
  TTS(ReadUnquoted, Edos_cr,     ReadDosCR,              { t.next_cell(); });
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace, { t.remember_whitespace(); });  // must remember whitespace in case its in the middle of the cell
  TTS(ReadUnquoted, Echar,       ReadUnquoted,           { t.add_whitespace(); t.add(); });
  TTS(ReadUnquoted, Eqchar,      ReadUnquoted,           { t.add_whitespace(); t.add(); });   // tolerant to quotes in the middle of unquoted cells
  TTS(ReadUnquoted, Ecomment,    ReadComment,            { t.next_cell(); t.end_row(); });
};



template <class TTrans>
class ST_ReadUnquotedWhitespace : public ST_Base<TTrans> {
public:
  // we have been reading some text, and we are working through some whitespace,
  // which could be in the middle of the text or at the end of the cell.
  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError,                { t.error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre,              { t.add_unquoted_post_whitespace(); t.next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start,                    { t.add_unquoted_post_whitespace(); t.next_cell(); t.end_row(); });
  TTS(ReadUnquotedWhitespace, Edos_cr,     ReadDosCR,                { t.add_unquoted_post_whitespace(); t.next_cell(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, ReadUnquotedWhitespace,   { t.remember_whitespace(); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted,             { t.add_whitespace(); t.add(); });
  TTS(ReadUnquotedWhitespace, Ecomment,    ReadComment,              { t.add_unquoted_post_whitespace(); t.next_cell(); t.end_row(); });
};



template <class TTrans>
class ST_ReadComment : public ST_Base<TTrans> {
public:
  TTS(ReadComment, Enewline,     Start,         {});  // end of line --> end of comment
  TTS(ReadComment, Echar,        ReadComment,   {});
  REDIRECT(ReadComment, Eqchar,       Echar)
  REDIRECT(ReadComment, Esep,         Echar)
  REDIRECT(ReadComment, Edos_cr,      Echar)
  REDIRECT(ReadComment, Ewhitespace,  Echar)
  REDIRECT(ReadComment, Ecomment,     Echar)
};



template <class TTrans>
class ST_ReadError : public ST_Base<TTrans> {
public:
  TTS(ReadError, Echar,       ReadError,  { assert(t.error_message); });
  REDIRECT(ReadError, Eqchar,      Echar)
  REDIRECT(ReadError, Esep,        Echar)
  REDIRECT(ReadError, Enewline,    Echar)
  REDIRECT(ReadError, Edos_cr,     Echar)
  REDIRECT(ReadError, Ewhitespace, Echar)
  REDIRECT(ReadError, Ecomment,    Echar)
};

#undef REDIRECT
#undef TTS

} // namespace csvFSM


// has template so users can specify either:
// * char (single quote/separator)
// * std::string (any number of)
// * boost::array<char,N> (exactly N possibles - should be faster for comparisons)
// * whatever! as long as we can call find(begin(X), end(X), char) so ADL might help
//   to resolve a custom begin/end/find.
//
// Note that I do not bother with unsigned-char, as even UTF-8 will work fine with
// the char type.
// If you use unsigned char in the template parameters, its likely that it won't
// compile as the templated functions are looking for a char, or assuming a 'container'.


// declare csv_parser as csvparser<Builder,Disable,etc>
// to disable quote support entirely - and to increase speed.
struct Disable {};
struct Separator_Comma {};


// support pre-C++11
template <class A, class B>
struct my_is_same { static const bool value = false; };

template <class A>
struct my_is_same<A,A> { static const bool value = true; };


template <class CsvBuilder, class QuoteChars = char, class Separators = char, class CommentChars = char>
class csv_parser
{
   // disable copy
   csv_parser& operator=(csv_parser const&);
   csv_parser(csv_parser const&);

public:
   static const bool FAST_commas_no_quotes_no_comments =
      my_is_same<Separators, Separator_Comma>::value
      &&
      my_is_same<QuoteChars, Disable>::value
      &&
      my_is_same<CommentChars, Disable>::value
      ;

   // static const bool builder_supported = csvFSM::BuilderSupported<CsvBuilder>::ok;

   typedef csvFSM::Trans<CsvBuilder> MyTrans;

   size_t current_row, current_column; // keeps track of where we are upto in the file

   // trim_whitespace: remove whitespace around unquoted cells
   // the standard says you should NOT trim, but many people would want to.
   // You can always quote the whitespace, and that will be kept

// constructor that was used before
csv_parser(CsvBuilder &out, QuoteChars qchar, Separators sep, bool trim_whitespace = false, bool collapse_separators = false)
 : qchar(qchar), sep(sep),
   comment(),  // inits comment char to zero if char
   comments_must_be_at_start_of_line(true),
   errmsg(NULL),
   collect_error_context(false),
   trans(out, trim_whitespace, collapse_separators)
{
   init_states();
   reset_cursor_location();
}


// construct for fast-path (no quotes, comments, fixed char separator
csv_parser(CsvBuilder &out, bool trim_whitespace = false, bool collapse_separators = false)
 : qchar(qchar), sep(sep),
   comment(),  // inits comment char to zero if char
   comments_must_be_at_start_of_line(true),
   errmsg(NULL),
   collect_error_context(false),
   trans(out, trim_whitespace, collapse_separators)
{
   init_states();
   reset_cursor_location();
}




// constructor with everything
// note: collect_error_context adds a small amount of overhead
csv_parser(CsvBuilder &out, QuoteChars qchar, Separators sep, bool trim_whitespace, bool collapse_separators, CommentChars comment, bool comments_must_be_at_start_of_line, bool collect_error_context = false)
 : qchar(qchar), sep(sep), comment(comment),
   comments_must_be_at_start_of_line(comments_must_be_at_start_of_line),
   errmsg(NULL),
   collect_error_context(collect_error_context),
   trans(out, trim_whitespace, collapse_separators)
{
   init_states();
   reset_cursor_location();
}


  ~csv_parser()
  {
     for (int i = 0; i < csvFSM::NUM_SI; ++i)
        delete state_trans[i];
  }
  
  // NOTE: returns true on error
bool operator()(const std::string &line) // not required to be linewise
{
  return process_chunk(line);
}


bool operator()(const char *&buf, size_t len)
{
   return process_chunk(buf,len);
}


void reset_cursor_location()
{
   current_row = 1;     // start at one, as we post-increment
   current_column = 0;  // start at zero, as we pre-increment
   trans.row_file_start_row = current_row;
}


size_t get_current_row() const
{
   return current_row;
}


size_t get_current_column() const
{
   return current_column;
}



bool process_chunk(const std::string &line) // not required to be linewise
{
  const char *buf=line.c_str();
  return process_chunk(buf,line.size());
}



// note: call this like so
// const char* temp = bufptr;
// process_chunk(temp,len);
// if there is an error, then temp is at the character that caused the problem.
bool process_chunk(const char *&buf, const size_t len)
{
  char const * const buf_end = buf + len;

  for ( ; buf != buf_end; ++buf ) {
     // note: current character is written directly to trans,
     // so that events become empty structs.
     trans.value = *buf;
     ++current_column;
     if (collect_error_context)
        current_row_content.push_back(*buf);

     using namespace csvFSM;

     switch (trans.value)
     {
        case '\0': {
                trans.error_message = "Unexpected NULL character"; // check for NULL character
                break;
             }

        case '\r': {
                state_idx = (state_trans[state_idx]->Edos_cr(trans));
                break;
             }

        case '\n': {
                trans.row_file_start_row = current_row;
                state_idx = (state_trans[state_idx]->Enewline(trans));
                if (collect_error_context)
                   current_row_content.clear();
                ++current_row;
                current_column = 0;
                break;
             }

        default: {
                if (!FAST_commas_no_quotes_no_comments && is_quote_char(qchar)) {
                   state_idx = (state_trans[state_idx]->Eqchar(trans));
                }

                else if (!FAST_commas_no_quotes_no_comments && match_char(sep)) {
                   state_idx = (state_trans[state_idx]->Esep(trans));
                }

                else if (!FAST_commas_no_quotes_no_comments && (!comments_must_be_at_start_of_line || trans.row_empty()) && match_char(comment)) {
                   // this one is more complex gate...
                   // only emit a comment event if comments can be anywhere or
                   // the row is still empty
                   state_idx = (state_trans[state_idx]->Ecomment(trans));
                }

                else
                {
                   switch (trans.value)
                   {
                      case ' ':
                      case '\t': {
                           state_idx = (state_trans[state_idx]->Ewhitespace(trans)); // check space
                           break;
                        }
  
                      case ',':  {
                           // fast-path will enable here, else, fall through to default
                           if (FAST_commas_no_quotes_no_comments) {
                              state_idx = (state_trans[state_idx]->Esep(trans));
                              break;
                           }
                        }

                      default: {
                           state_idx = (state_trans[state_idx]->Echar(trans));
                           break;
                        }
                   }
               }
           }
     }

    if (trans.error_message) {
#if CPPCSV_DEBUG
       fprintf(stderr, "State index: %d\n", state.which());
       fprintf(stderr,"csv parse error: %s\n",error());
#endif
      return true;
    }
  }
  return false;
}



// helper function, assumes when len=0 then we need to end the line
// ie we hit the end of the input file.
//
// This will call flush() if len=0
//
// Allows you to change code like this:
//
// n = fread(..);
// if (n == 0) {
//    if (csv.flush()) handle error;
// }
// else {
//    if (csv.process_chunk(buf,n)) handle error;
// }
//
//    to:
//
// n = fread(..);
// if (csv.process(buf,n)) handle error;
//
bool process(const char *&buf, const size_t len)
{
   if (len == 0)
      return flush();
   return process_chunk(buf, len);
}




// call this after processing the last chunk
// this will help when the input data did not finish with a newline
bool flush()
{
  if (trans.is_row_open()) {
    using namespace csvFSM;
    trans.row_file_start_row = current_row;
    state_idx = (state_trans[state_idx]-> Enewline(trans) );
  }
  return (trans.error_message != NULL);
}



  // note: returns NULL if no error
  const char * error() const { return trans.error_message; }

  std::string error_context() const {
     if (current_column == 0)
        return std::string();
     return current_row_content + "\n" + std::string(current_column-1,'-') + "^";
  }

private:
   void init_states()
   {
      state_trans[csvFSM::Start] = new csvFSM::ST_Start<MyTrans>();
      state_trans[csvFSM::ReadSkipPre] = new csvFSM::ST_ReadSkipPre<MyTrans>();
      state_trans[csvFSM::ReadQuoted] = new csvFSM::ST_ReadQuoted<MyTrans>();
      state_trans[csvFSM::ReadQuotedCheckEscape] = new csvFSM::ST_ReadQuotedCheckEscape<MyTrans>();
      state_trans[csvFSM::ReadQuotedSkipPost] = new csvFSM::ST_ReadQuotedSkipPost<MyTrans>();
      state_trans[csvFSM::ReadDosCR] = new csvFSM::ST_ReadDosCR<MyTrans>();
      state_trans[csvFSM::ReadQuotedDosCR] = new csvFSM::ST_ReadQuotedDosCR<MyTrans>();
      state_trans[csvFSM::ReadUnquoted] = new csvFSM::ST_ReadUnquoted<MyTrans>();
      state_trans[csvFSM::ReadUnquotedWhitespace] = new csvFSM::ST_ReadUnquotedWhitespace<MyTrans>();
      state_trans[csvFSM::ReadComment] = new csvFSM::ST_ReadComment<MyTrans>();
      state_trans[csvFSM::ReadError] = new csvFSM::ST_ReadError<MyTrans>();

      state_idx = csvFSM::Start;
   }

  QuoteChars qchar;  // could be char or string
  Separators sep;    // could be char or string
  CommentChars comment;   // could be char or string
  bool comments_must_be_at_start_of_line;
  const char *errmsg;

  bool collect_error_context;
  std::string current_row_content;  // remember what we read for error printouts

  csvFSM::StateIdx state_idx;
  csvFSM::ST_Base<MyTrans> * state_trans[csvFSM::NUM_SI];
  MyTrans trans;

  // support either single or multiple quote characters
  bool is_quote_char( char the_qchar ) const
  {
    return match_char(the_qchar);
  }

  bool is_quote_char( Disable ) const
  {
    return false;
  }

  template <class Container>
  bool is_quote_char( Container const& the_qchar ) const
  {
    if (trans.active_qchar != 0)
       return match_char(trans.active_qchar);
    return match_char(the_qchar);
  }

  // support either a single char or a string of chars

  // note: ignores the null (0) character (ie if there is no comment char)
  bool match_char( Disable ) const {
     return false;
  }

  bool match_char( Separator_Comma ) const {
     assert(0);   // should not actually be called, should be picked up by switch first
     return trans.value == ',';
  }

  bool match_char( char target ) const {
     return target != 0 && target == trans.value;
  }

  template <class Container>
  bool match_char( Container const& target ) const {
     // boost not needed ...
     // using boost::range::find;
     // return find(target, trans.value) != boost::end(target);
     return std::find(target.begin(), target.end(), trans.value) != target.end();
  }
};


}
