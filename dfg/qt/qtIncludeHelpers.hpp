#pragma once

#ifdef _MSC_VER
	#define DFG_BEGIN_INCLUDE_QT_HEADERS \
		__pragma(warning(push)) \
		__pragma(warning(disable:4127)) /* "Conditional expression is constant" */ \
		__pragma(warning(disable:4231)) /* "nonstandard extension used" */ \
		__pragma(warning(disable:4244)) /* "conversion from 'x' to 'y', possible loss of data" */ \
		__pragma(warning(disable:4251)) /* "class 'x' needs to have dll-interface to be used by clients of class 'y'" */ \
		__pragma(warning(disable:4275)) /* "non dll-interface class 'x' used as base for dll-interface class 'y'" */ \
		__pragma(warning(disable:4512)) /* "assignment operator could not be generated" */ \
		__pragma(warning(disable:4640)) /* "construction of local static object is not thread-safe" */ \
		__pragma(warning(disable:4800)) /* "forcing value to bool 'true' or 'false' (performance warning)" */ \
		__pragma(warning(disable:4826)) /* "Conversion from 'x' to 'y' is sign-extended. This may cause unexpected runtime behavior." */ \

	#define DFG_END_INCLUDE_QT_HEADERS \
		__pragma(warning(pop))

#else
	#define DFG_BEGIN_INCLUDE_QT_HEADERS
	#define DFG_END_INCLUDE_QT_HEADERS
#endif
