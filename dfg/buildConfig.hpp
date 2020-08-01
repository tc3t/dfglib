#ifndef DFG_BUILDCONFIG_NPQAEA25
#define DFG_BUILDCONFIG_NPQAEA25

#include "dfgDefs.hpp"
#include "preprocessor/compilerInfoMsvc.hpp"

#ifdef _MSC_VER
	//#define _CRT_SECURE_NO_WARNINGS		// Define to disable the "This function or variable may be unsafe" warnings.
	#ifndef _SCL_SECURE_NO_WARNINGS
		#define _SCL_SECURE_NO_WARNINGS		// Define to disable warnings on calling "potentially unsafe methods in the Standard C++ Library". These include for example std::copy(in VC2008).
	#endif

	#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES	
		#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES			1
	#endif

	#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT
		#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT	1
	#endif

#endif // _MSC_VER

#if (!defined(NOMINMAX) && defined(_WIN32) && (!defined(DFG_NOWINMINMAX) || DFG_NOWINMINMAX != 0))
	#define NOMINMAX						// Suppress inclusion of min/max macros from Windows headers.
#endif

#if !defined(DFG_IS_QT_AVAILABLE)
	//#if defined(QT_VERSION)
	#if defined(QT_CORE_LIB) || defined(QT_VERSION)
		#define DFG_IS_QT_AVAILABLE	1
	#else
		#define DFG_IS_QT_AVAILABLE	0
	#endif
#endif


#if (DFG_MSVC_VER > 0)
	// Enable some 'off by default'-warnings

	//#pragma warning(default : 4619) // "#pragma warning : there is no warning number 'number' "

	#pragma warning(default : 4242) // "'identifier': conversion from 'type1' to 'type2', possible loss of data"
	#pragma warning(default : 4263) // "'function' : member function does not override any base class virtual member function"
	#pragma warning(default : 4264) // "'virtual_function' : no override available for virtual member function from base 'class'; function is hidden "
	#pragma warning(default : 4265) // "'class': class has virtual functions, but destructor is not virtual"
	#pragma warning(default : 4266)  // "'function' : no override available for virtual member function from base 'type'; function is hidden"
	#pragma warning(default : 4302) // "'conversion': truncation from 'type1' to 'type2'"

	#pragma warning(default : 4826) // "conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior"
	#pragma warning(default : 4905) // "wide string literal cast to 'LPSTR'"
	#pragma warning(default : 4906) // "string literal cast to 'LPWSTR'"
	#pragma warning(default : 4928) // "illegal copy-initialization; more than one user-defined conversion has been implicitly applied"

	#pragma warning(default : 4254) // "'operator' : conversion from 'type1' to 'type2', possible loss of data, A larger bit field was assigned to a smaller bit field. There could be a loss of data."
	#pragma warning(default : 4640) // "'instance' : construction of local static object is not thread-safe "

	// Commented out because it causes too many warnings from Windows includes.
	//#pragma warning(default : 4668) // "'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives' "

	// Commented out because it causes too many warnings.
	//#pragma warning(default : 4710) // "'function' : function not inlined "

	#pragma warning(default : 4711) // "function 'function' selected for inline expansion "
	#pragma warning(default : 4738) // "storing 32-bit float result in memory, possible loss of performance "

	// Commented out because it causes too many warnings from Windows includes.
	//#pragma warning(default : 4820) // "'bytes' bytes padding added after construct 'member_name' "

	#pragma warning(default : 4826) // "Conversion from 'type1 ' to 'type_2' is sign-extended. This may cause unexpected runtime behavior."
	#pragma warning(default : 4836) // "nonstandard extension used : 'type' : local types or unnamed types cannot be used as template arguments"
	#pragma warning(default : 4905) // "wide string literal cast to 'LPSTR' "
	#pragma warning(default : 4906) // "string literal cast to 'LPWSTR' "
	#pragma warning(default : 4946) // "reinterpret_cast used between related classes: 'class1' and 'class2'"

	// Disabling some warnings
	#pragma warning(disable : 4481) // "nonstandard extension used: override specifier 'keyword'"
	#pragma warning(disable : 4512) // "assignment operator could not be generated". 

	// Treating some warnings as error:
	#pragma warning(error : 4307) // "integral constant overflow"
	#pragma warning(error : 4717) // "recursive on all control paths, function will cause runtime stack overflow"

#endif // (DFG_MSVC_VER > 0)
 
#endif // include guard
