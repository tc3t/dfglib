#pragma once

// Using macro since template implementation even with very simple lambdas seemed to easily fail
// to vectorize both in VC2012 and VC2013.
// Note: While this macro is global library macro, this is internal implementation detail.

#define DFG_ZIMPL_VECTORIZING_LOOP(ARR, SIZE, DEF) \
	for(size_t i = 0; i < SIZE; ++i) \
		DEF;

#define DFG_ZIMPL_VECTORIZING_LOOP_RHS(ARR, SIZE, RHS) \
    DFG_ZIMPL_VECTORIZING_LOOP(ARR, SIZE, ARR[i] RHS)
