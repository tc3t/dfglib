#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"

#ifdef _MSC_VER
	#include <Windows.h> // OutputDebugString
#endif

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(debug) {

#ifdef _MSC_VER

	namespace DFG_DETAIL_NS
	{
		template <class Func, class Char_T>
		void printToDebugDisplayImpl(Func func, const Char_T* psz, const bool bAddNewLine = true)
		{
			func(psz);
			if (bAddNewLine)
			{
				const Char_T szEol[2] = { '\n', '\0' };
				func(szEol);
			}
		}
	}

	inline void printToDebugDisplay(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamC) s, const bool bAddNewLine = true)
	{
		DFG_DETAIL_NS::printToDebugDisplayImpl(OutputDebugStringA, s.c_str(), bAddNewLine);
	}

	inline void printToDebugDisplay(DFG_ROOT_NS::DFG_CLASS_NAME(ReadOnlySzParamW) s, const bool bAddNewLine = true)
	{
		DFG_DETAIL_NS::printToDebugDisplayImpl(OutputDebugStringW, s.c_str(), bAddNewLine);
	}

#endif // _MSC_VER

} } // module debug
