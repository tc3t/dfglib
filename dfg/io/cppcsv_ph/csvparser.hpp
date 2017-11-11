// Copyright (c) 2013 Tobias Hoffmann
//               2013 Paul Harris <paulharris@computer.org>
//
// License: http://opensource.org/licenses/MIT

#include <cassert>
#include "csvbase.hpp"

#include <string>
#include <vector>

#include <boost/range/algorithm/find.hpp>
#include <boost/range/end.hpp>
#include <boost/utility.hpp>
#include <boost/variant.hpp>

#define CPPCSV_DEBUG 0

#if CPPCSV_DEBUG
  #include <cstdio>
  #include <typeinfo>
#endif

namespace cppcsv {
namespace csvFSM {

// States - must all be emptry structs
struct Start {};
struct ReadSkipPre {};
struct ReadQuoted {};
struct ReadQuotedCheckEscape {};
struct ReadQuotedSkipPost {};
struct ReadDosCR {};
struct ReadQuotedDosCR {};
struct ReadUnquoted {};
struct ReadUnquotedWhitespace {};
struct ReadComment {};
struct ReadError {};

typedef boost::variant<Start,
                       ReadDosCR,
                       ReadQuotedDosCR,
                       ReadSkipPre,
                       ReadQuoted,
                       ReadQuotedCheckEscape,
                       ReadQuotedSkipPost,
                       ReadUnquoted,
                       ReadUnquotedWhitespace,
                       ReadComment,
                       ReadError> States;

// Events
struct Echar {};
struct Ewhitespace {};
struct Eqchar {};
struct Esep {};
struct Enewline {};
struct Edos_cr {};   // DOS carriage return
struct Ecomment {};


// compile-time interface switches
/*
template <class T> struct BuilderSupported;
template <> struct BuilderSupported<per_cell_tag> { static const bool ok = true; };
template <> struct BuilderSupported<per_row_tag>  { static const bool ok = true; };
*/

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
struct Trans {
  Trans(CsvBuilder &out, bool trim_whitespace, bool collapse_separators ) :
     value(0),
     row_file_start_row(0),
     active_qchar(0),
     error_message(NULL),
     out(out),
     trim_whitespace(trim_whitespace),
     collapse_separators(collapse_separators)
   {}

  // this is set before the state change is called,
  // that way the Events do not need to carry their state with them.
  char value;

  size_t row_file_start_row;      // first file row for this row

  // active quote character, required in the situation where multiple quote chars are possible
  // if =0 then there is no active_qchar
  char active_qchar;

private:
  template <class Event>
  struct NextState : boost::static_visitor<States> {
    NextState(Trans &t) : t(t) {}

    // note: states are just empty structs, so pass by copy should be faster
    template <class State>
    States operator()(State s) const {
#if CPPCSV_DEBUG
  printf("%s %c (%d)\n",typeid(State).name(), t.value, (int)t.value);
#endif
      return t.on(s,Event());
    }

  private:
    Trans &t;
  };


public:
   // finds the next state, performs transition activity, sets the new state
  template <class State, class Event>
  States process_event( State state, Event event ) {
    return boost::apply_visitor( NextState<Event>(*this), state);
  }


  bool row_empty() const {
     return cells_buffer.empty() && whitespace_state.empty();
  }

  size_t last_cell_length() const {
     assert(is_row_open());
     return cells_buffer.size() - cell_offsets.back();
  }

  bool is_row_open() const {
     return !cell_offsets.empty();
  }

  const char* error_message;

    // note: states and events are just empty structs, so pass by copy should be faster

  // for defining a state transition with code
#define TTS(State, Event, TargetState, code) States on(State, Event) { code; return TargetState(); }

  // for defining a transition that redirects to another Event handler for this state
#define REDIRECT(State, Event, OtherEvent) States on(State state, Event) { return on(state,OtherEvent()); }

  //  State          Event        Next_State    Transition_Action
  TTS(Start,         Eqchar,      ReadQuoted,   { active_qchar = value; begin_row(); });
  TTS(Start,         Esep,        ReadSkipPre,  { begin_row(); next_cell(false); });
  TTS(Start,         Enewline,    Start,        { begin_row(); end_row(); });
  TTS(Start,         Edos_cr,     ReadDosCR,   {});
  TTS(Start,         Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } begin_row(); });   // we MAY want to remember this whitespace
  TTS(Start,         Echar,       ReadUnquoted, { begin_row(); add(); });
  TTS(Start,         Ecomment,    ReadComment,  {});  // comment at the start of the line --> everything discarded, no blank row is generated

  TTS(ReadDosCR,    Eqchar,    ReadError, { error_message = "quote after CR"; });
  TTS(ReadDosCR,    Esep,      ReadError, { error_message = "sep after CR"; });
  // If row is blank, begin and then end it.
  TTS(ReadDosCR,    Enewline,  Start,     { if (!is_row_open()) { begin_row(); } end_row(); });
  TTS(ReadDosCR,    Edos_cr,   ReadError, { error_message = "CR after CR"; });
  TTS(ReadDosCR,    Ewhitespace, ReadError, { error_message = "whitespace after CR"; });
  TTS(ReadDosCR,    Echar,     ReadError, { error_message = "char after CR"; });
  TTS(ReadDosCR,    Ecomment,  ReadError, { error_message = "comment after CR"; });

  TTS(ReadSkipPre,   Eqchar,      ReadQuoted,   { active_qchar = value; drop_whitespace(); });   // we always want to forget whitespace before the quotes
  TTS(ReadSkipPre,   Esep,        ReadSkipPre,  { if (!collapse_separators) { next_cell(false); } });
  TTS(ReadSkipPre,   Enewline,    Start,        { next_cell(false); end_row(); });
  TTS(ReadSkipPre,   Edos_cr,     ReadDosCR,    { next_cell(false); });  // same as newline, except we expect to see newline next
  TTS(ReadSkipPre,   Ewhitespace, ReadSkipPre,  { if (!trim_whitespace) { remember_whitespace(); } });  // we MAY want to remember this whitespace
  TTS(ReadSkipPre,   Echar,       ReadUnquoted, { add_whitespace(); add(); });   // we add whitespace IF any was recorded
  TTS(ReadSkipPre,   Ecomment,    ReadComment,  { add_whitespace(); if (last_cell_length() != 0) { next_cell(true); } end_row(); } );   // IF there was anything, then emit a cell else completely ignore the current cell (ie do not emit a null)

  TTS(ReadQuoted,    Eqchar,      ReadQuotedCheckEscape, {});
  TTS(ReadQuoted,    Edos_cr,     ReadQuotedDosCR,       {});  // do not add, we translate \r\n to \n, even within quotes
  TTS(ReadQuoted,    Echar,       ReadQuoted,            { add(); });
  REDIRECT(ReadQuoted,    Esep,        Echar )
  REDIRECT(ReadQuoted,    Enewline,    Echar )
  REDIRECT(ReadQuoted,    Ewhitespace, Echar )
  REDIRECT(ReadQuoted,    Ecomment,    Echar )

  TTS(ReadQuotedDosCR,    Eqchar,    ReadError,    { error_message = "quote after CR"; });
  TTS(ReadQuotedDosCR,    Esep,      ReadError,    { error_message = "sep after CR"; });
  TTS(ReadQuotedDosCR,    Enewline,  ReadQuoted,   { add(); });   // we see \r\n, so add(\n) and continue reading Quoted
  TTS(ReadQuotedDosCR,    Edos_cr,   ReadError,    { error_message = "CR after CR"; });
  TTS(ReadQuotedDosCR,    Ewhitespace, ReadError,  { error_message = "whitespace after CR"; });
  TTS(ReadQuotedDosCR,    Echar,     ReadError,    { error_message = "char after CR"; });
  TTS(ReadQuotedDosCR,    Ecomment,  ReadError,    { error_message = "comment after CR"; });

  // we are reading quoted text, we see a "... here we are looking to see if its followed by another "
  // if so, then output a quote, else its the end of the quoted section.
  TTS(ReadQuotedCheckEscape, Eqchar,      ReadQuoted,          { add(); });
  TTS(ReadQuotedCheckEscape, Esep,        ReadSkipPre,         { active_qchar = 0; next_cell(); });
  TTS(ReadQuotedCheckEscape, Enewline,    Start,               { active_qchar = 0; next_cell(); end_row(); });
  TTS(ReadQuotedCheckEscape, Edos_cr,     ReadDosCR,           { active_qchar = 0; next_cell(); });
  TTS(ReadQuotedCheckEscape, Ewhitespace, ReadQuotedSkipPost,  { active_qchar = 0; });
  TTS(ReadQuotedCheckEscape, Echar,       ReadError,           { error_message = "char after possible endquote"; });
  TTS(ReadQuotedCheckEscape, Ecomment,    ReadComment,         { active_qchar = 0; next_cell(); end_row(); });

  // we have finished a quoted cell, we are just looking for the next separator or end point so we can emit the cell
  TTS(ReadQuotedSkipPost, Eqchar,      ReadError,           { error_message = "quote after endquote"; });
  TTS(ReadQuotedSkipPost, Esep,        ReadSkipPre,         { next_cell(); });
  TTS(ReadQuotedSkipPost, Enewline,    Start,               { next_cell(); end_row(); });
  TTS(ReadQuotedSkipPost, Edos_cr,     ReadDosCR,           { next_cell(); });
  TTS(ReadQuotedSkipPost, Ewhitespace, ReadQuotedSkipPost,  {});
  TTS(ReadQuotedSkipPost, Echar,       ReadError,           { error_message = "char after endquote"; });
  TTS(ReadQuotedSkipPost, Ecomment,    ReadComment,         { next_cell(); end_row(); });

  // we have seen some text, and we are reading it in
  // TTS(ReadUnquoted, Eqchar,      ReadError,              { error_message = "unexpected quote in unquoted string"; });
  // Modified, if we see a quote in an unquoted cell, then just read as a plain quote
  // The rule of always quoting cells with quotes is a "please do" but Excel does not practice it when writing to Clipboard, and is tolerant when reading.
  // So Eqchar is hangled in the same way as Echar (below)

  TTS(ReadUnquoted, Esep,        ReadSkipPre,            { next_cell(); });
  TTS(ReadUnquoted, Enewline,    Start,                  { next_cell(); end_row(); });
  TTS(ReadUnquoted, Edos_cr,     ReadDosCR,              { next_cell(); });
  TTS(ReadUnquoted, Ewhitespace, ReadUnquotedWhitespace, { remember_whitespace(); });  // must remember whitespace in case its in the middle of the cell
  TTS(ReadUnquoted, Echar,       ReadUnquoted,           { add_whitespace(); add(); });
  TTS(ReadUnquoted, Eqchar,      ReadUnquoted,           { add_whitespace(); add(); });   // tolerant to quotes in the middle of unquoted cells
  TTS(ReadUnquoted, Ecomment,    ReadComment,            { next_cell(); end_row(); });

  // we have been reading some text, and we are working through some whitespace,
  // which could be in the middle of the text or at the end of the cell.
  TTS(ReadUnquotedWhitespace, Eqchar,      ReadError,                { error_message = "unexpected quote after unquoted string"; });
  TTS(ReadUnquotedWhitespace, Esep,        ReadSkipPre,              { add_unquoted_post_whitespace(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Enewline,    Start,                    { add_unquoted_post_whitespace(); next_cell(); end_row(); });
  TTS(ReadUnquotedWhitespace, Edos_cr,     ReadDosCR,                { add_unquoted_post_whitespace(); next_cell(); });
  TTS(ReadUnquotedWhitespace, Ewhitespace, ReadUnquotedWhitespace,   { remember_whitespace(); });
  TTS(ReadUnquotedWhitespace, Echar,       ReadUnquoted,             { add_whitespace(); add(); });
  TTS(ReadUnquotedWhitespace, Ecomment,    ReadComment,              { add_unquoted_post_whitespace(); next_cell(); end_row(); });

  TTS(ReadError, Echar,       ReadError,  { assert(error_message); });
  REDIRECT(ReadError, Eqchar,      Echar)
  REDIRECT(ReadError, Esep,        Echar)
  REDIRECT(ReadError, Enewline,    Echar)
  REDIRECT(ReadError, Edos_cr,     Echar)
  REDIRECT(ReadError, Ewhitespace, Echar)
  REDIRECT(ReadError, Ecomment,    Echar)

  TTS(ReadComment, Enewline,     Start,         {});  // end of line --> end of comment
  TTS(ReadComment, Echar,        ReadComment,   {});
  REDIRECT(ReadComment, Eqchar,       Echar)
  REDIRECT(ReadComment, Esep,         Echar)
  REDIRECT(ReadComment, Edos_cr,      Echar)
  REDIRECT(ReadComment, Ewhitespace,  Echar)
  REDIRECT(ReadComment, Ecomment,     Echar)

#undef REDIRECT
#undef TTS

private:
  // adds current character to cell buffer
  void add()
  {
     cells_buffer.push_back(value);
  }

  // whitespace is remembered until we know if we need to add to the output or forget
  void remember_whitespace()
  {
     whitespace_state.push_back(value);
  }

  // adds remembered whitespace to cell buffer
  void add_whitespace()
  {
     cells_buffer.append(whitespace_state);
     drop_whitespace();
  }

  // only writes out whitespace IF we aren't trimming
  void add_unquoted_post_whitespace()
  {
     if (!trim_whitespace)
        cells_buffer.append(whitespace_state);
     drop_whitespace();
  }

  void drop_whitespace()
  {
     whitespace_state.clear(); 
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
    const size_t start_off = cell_offsets.back();
    const size_t end_off = cells_buffer.size();

    cell_offsets.push_back(end_off);

    if (has_content) {
      assert(!cells_buffer.empty());
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
     char * buffer = (cells_buffer.empty() ? NULL : &cells_buffer[0]);

     call_out_end_full_row(
           out, out,
           buffer,                  // buffer
           cell_offsets.size()-1,   // num cells
           &cell_offsets[0],        // offsets
           row_file_start_row       // first file row for this row
           );

     cell_offsets.clear();
     cells_buffer.clear();
  }

  CsvBuilder &out;
  std::string cells_buffer;
  std::vector<size_t> cell_offsets;
  std::string whitespace_state;
  bool trim_whitespace;
  bool collapse_separators;
};


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

template <class CsvBuilder, class QuoteChars = char, class Separators = char, class CommentChars = char>
struct csv_parser : boost::noncopyable {

   // static const bool builder_supported = csvFSM::BuilderSupported<CsvBuilder>::ok;

   typedef csvFSM::Trans<CsvBuilder> MyTrans;

   size_t current_row, current_column; // keeps track of where we are upto in the file

   // trim_whitespace: remove whitespace around unquoted cells
   // the standard says you should NOT trim, but many people would want to.
   // You can always quote the whitespace, and that will be kept

// constructor that was used before
csv_parser(CsvBuilder &out, QuoteChars qchar, Separators sep, bool trim_whitespace = false, bool collapse_separators = false)
 : qchar(qchar), sep(sep), comment(0),
   comments_must_be_at_start_of_line(true),
   errmsg(NULL),
   collect_error_context(false),
   trans(out, trim_whitespace, collapse_separators)
{
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
   reset_cursor_location();
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

         if (trans.value == '\0')   trans.error_message = "Unexpected NULL character"; // check for NULL character

    else if (match_char('\r'))      process_event(Edos_cr());

    else if (match_char('\n'))      {
       trans.row_file_start_row = current_row;
       process_event(Enewline());
       if (collect_error_context)
          current_row_content.clear();
       ++current_row;
       current_column = 0;
    }

    else if (is_quote_char(qchar))  process_event(Eqchar());

    else if (match_char(sep))       process_event(Esep());

    else if ( (!comments_must_be_at_start_of_line || trans.row_empty()) && match_char(comment)) {
       // this one is more complex gate... only emit a comment event if comments can be anywhere or
       // the row is still empty
       process_event(Ecomment());
    }

    else if (match_char(' '))       process_event(Ewhitespace()); // check space
    else if (match_char('\t'))      process_event(Ewhitespace()); // check tab
    else                            process_event(Echar());

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
    process_event( Enewline() );
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
  QuoteChars qchar;  // could be char or string
  Separators sep;    // could be char or string
  CommentChars comment;   // could be char or string
  bool comments_must_be_at_start_of_line;
  const char *errmsg;

  bool collect_error_context;
  std::string current_row_content;  // remember what we read for error printouts

  csvFSM::States state;
  MyTrans trans;

  // support either single or multiple quote characters
  bool is_quote_char( char the_qchar ) const
  {
    return match_char(the_qchar);
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
  bool match_char( char target ) const {
     return target != 0 && target == trans.value;
  }

  template <class Container>
  bool match_char( Container const& target ) const {
    using boost::range::find;
    return find(target, trans.value) != boost::end(target);
  }


   // finds the next state, performs transition activity, sets the new state
   template <class Event>
   void process_event( Event event ) {
     state = trans.process_event(state, event);
   }
};


}
