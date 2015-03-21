#pragma once

#include <string>

DFG_ROOT_NS_BEGIN { // Note: String is included in main namespace on purpose.

/*
template <class Char_T> class String_T : public std::basic_string<Char_T> {};

typedef String_T<char>		StringA;
typedef String_T<wchar_t>	StringW;
*/

typedef std::string		StringA;
typedef std::wstring	StringW;

#if defined(_WIN32)
	#if defined(_UNICODE)
		typedef StringW String;
	#else
		typedef StringA String;
	#endif
#endif

} // root namespace




