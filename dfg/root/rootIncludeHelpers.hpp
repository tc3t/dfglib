#pragma once

#define DFG_BEGIN_INCLUDE_ROOT_HEADERS \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4996)) /* "This function or variable may be unsafe."*/ \
	__pragma(warning(disable : 4800)) /* "forcing value to bool 'true' or 'false' (performance warning)" */ \
	__pragma(warning(disable : 4127)) /* "Conditional expression is constant" */ \
	__pragma(warning(disable : 4267))  /* 'argument' : conversion from 'size_t' to 'Ssiz_t', possible loss of data */ \
	__pragma(warning(disable : 4302)) /* 'argument' : 'type cast' : truncation from 'Event_t *' to 'Long_t' */


#define END_INCLUDE_ROOT_HEADERS \
	__pragma(warning(pop))
