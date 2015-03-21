#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN
{
	namespace DFG_DETAIL_NS
	{
		template <class T>
		struct BuildFailureHelper
		{
			static const bool value = true;
		};
	}
}

/*
 Causes static_assert(false, msg) (or similar).
 Intended for template functions that should not be instantiated.
 Simply writing static_assert(false, ""); inside the function works in VC10 but not in GCC 4.8.0
 T should be a template parameter of the template function.
 Example:
 template <class T> void func(T)
 {
	DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "This function is not implemented for given type");
 }

 template <> void func(int) {}

*/
#define DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, msg) \
	DFG_STATIC_ASSERT(DFG_ROOT_NS::DFG_DETAIL_NS::BuildFailureHelper<T>::value != true, msg);
