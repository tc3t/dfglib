#pragma once

#include "dfgDefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(memory) {

#if defined(_MSC_VER) && (_MSC_VER >= 1500 /*VC2008*/)
	// Stack memory allocation.
	// DFG_STACK_ALLOC(nBytes) "returns" void* which should be freed with DFG_STACK_FREE().
	// Note: These macros do not quarantee that the memory gets allocated from the stack.
	//		 To be considered as requests for stack allocation which may or may not be fullfilled.
	#define DFG_STACK_ALLOC(x)	_malloca(x)
	#define DFG_STACK_FREE(x)	_freea(x)
#endif

}} // module memory
