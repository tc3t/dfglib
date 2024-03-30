#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN {


	// Fixed Capacity string. Has internal character array which can't be exceeded.
	// Aimed as a lightweight return type for functions that return string whose maximum 
	// length limit is known. This avoids (redundant) memory allocation that might occur if using string objects.
	template <class Char_T, size_t MaxStrLength_T> class StringFixedCapacity_T
	{
	public:
		static const size_t s_nMaxStrLength = MaxStrLength_T;

		typedef Char_T CharBuffer[s_nMaxStrLength + 1];

		operator CharBuffer&() {return m_sz;}
		operator const CharBuffer&() const {return m_sz;}

		Char_T* data() { return &m_sz[0]; }
		const Char_T* data() const { return &m_sz[0]; }

		static size_t capacity() {return s_nMaxStrLength;}
		const Char_T* c_str() const {return m_sz;}

		StringFixedCapacity_T() {m_sz[0] = 0;}
	private:
		CharBuffer m_sz;
	};

} // namespace DFG_ROOT_NS
