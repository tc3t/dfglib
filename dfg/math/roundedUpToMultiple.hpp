#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

	// For positive integer 'val', returns the smallest value greater or equal to 'val' which 'k' divides.
	// If k zero, returns 'val'.
	// If the mathematical value does not fit into given data type, unsigned overflow will result.
	inline size_t roundedUpToMultiple(size_t val, size_t k)
	{
		if (k == 0 || k == 1)
			return val;
		DFG_ASSERT_CORRECTNESS(val % k == 0 || val + (k - (val % k)) == (val/k + 1)*k);
		return (val % k == 0) ? val : val + (k - (val % k));
	}

	template <size_t val_T, size_t k_T> struct DFG_CLASS_NAME(RoundedUpToMultipleT) {static const size_t value = val_T + (size_t(k_T - size_t(val_T % k_T)) % k_T);};

}} // module math
