#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//
//
// Bit manipulation
//
//
//
///////////////////////////////////////////////////////////////////////////////

#include "dfgBase.hpp"
#include "bits/byteSwap.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(bits) {

// Makes values from set bit indexes.
// Examples: valueBySetBits(0) = bin(1) = 1
//			  valueBySetBits(1) = bin(10) = 2
//			  valueBySetBits(4, 1) == bin(10010) = 18
// Note: Indexes may be equal, e.g. BitsToValue(1, 1) = BitsToValue(1)
// TODO: test
inline unsigned int valueBySetBits(const int i1) {return (1u << i1);}
inline unsigned int valueBySetBits(const int i1, const int i2) {return valueBySetBits(i2) | (1u << i1);}
inline unsigned int valueBySetBits(const int i1, const int i2, const int i3) {return valueBySetBits(i2, i3) | (1u << i1);}
inline unsigned int valueBySetBits(const int i1, const int i2, const int i3, const int i4) {return valueBySetBits(i2, i3, i4) | (1u << i1);}
inline unsigned int valueBySetBits(const int i1, const int i2, const int i3, const int i4, const int i5) {return valueBySetBits(i2, i3, i4, i5) | (1u << i1);}
inline unsigned int valueBySetBits(const int i1, const int i2, const int i3, const int i4, const int i5, const int i6) {return valueBySetBits(i2, i3, i4, i5, i6) | (1u << i1);}

// Tests bit in index nBitIndex.
// TODO: test
// Remarks: in WinNT.h there's define BitTest _bittest
template <class Int_T, class Index_T>
inline bool bitTest(const Int_T val, const Index_T nBitIndex)
{
	return ((val & (valueBySetBits(nBitIndex))) != 0);
}

// Returns value with bit in index 'bitIndex' set to state 'flagValue' and returns the result.
// TODO: Test
template <class Int_T>
Int_T bitSetted(Int_T val, const Int_T nBitIndex, const bool flagValue)
{
	// In MSVC, see _bittestandset.
	if(flagValue)
		return (val |= valueBySetBits(nBitIndex));
	else
		return (val &= ~(valueBySetBits(nBitIndex)));
}

}} // module bits
