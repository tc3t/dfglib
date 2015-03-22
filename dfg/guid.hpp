#pragma once

#include "dfgDefs.hpp"
#include "str/stringFixedCapacity.hpp"
#include "dfgAssert.hpp"

#ifdef _WIN32
	#include <Objbase.h> // CoCreateGuid
	#include <Rpc.h>	 // RpcStringFree

	#pragma comment(lib, "Rpcrt4")	// For GUID-related functions.
#endif // _WIN32

DFG_ROOT_NS_BEGIN {

#ifdef _WIN32

//===================================================================
// Class: GuidCreator
// Description: Creates GUIDs using WinAPI
// Example: GuidCreator().ToStr<TCHAR>();
class DfGuidCreator
//===================================================================
{
public:
	// Returns version 4 GUID.
	template <class Char_T> inline DFG_CLASS_NAME(StringFixedCapacity_T)<Char_T, 36> V4_ToStr()
	{
		// GUID is 36 chars + NULL.
		GUID guid;
		DFG_CLASS_NAME(StringFixedCapacity_T)<Char_T, 36> string;
		if (CoCreateGuid(&guid) == S_OK)
		{
			BYTE* str;
			UuidToStringA(&guid, &str);
			const char* psz = reinterpret_cast<const char*>(str);
			const auto nPszLength = strlen(psz);
			DFG_ASSERT_CORRECTNESS(nPszLength == 36);
			// Check GUID type: expect version 4 UUID (Source: Wikipedia, "Universally unique identifier")
			DFG_ASSERT_CORRECTNESS(str[14] == '4'); //
			DFG_ASSERT_CORRECTNESS(str[19] == '8' || str[19] == '9' || str[19] == 'A' || str[19] == 'B'
							|| str[19] == 'a' || str[19] == 'b');
			for(size_t i = 0; i<nPszLength; ++i)
				string[i] = static_cast<Char_T>(psz[i]);
			RpcStringFreeA(&str);
		}
		return string;
	}
};

#endif // _WIN32

}

