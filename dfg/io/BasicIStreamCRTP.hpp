#pragma once

#include "../dfgBase.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

// CRTP base class for BasicIStream classes.
template <class Impl_T, class Char_T = char>
class DFG_CLASS_NAME(BasicIStreamCRTP)
{
public:
	typedef typename std::char_traits<Char_T> traits_type; // TODO: revise this.
	typedef typename traits_type::int_type int_type;
	typedef fpos_t PosType;

	DFG_CLASS_NAME(BasicIStreamCRTP)()
	{
	}

	~DFG_CLASS_NAME(BasicIStreamCRTP)()
	{
	}

	int_type eofVal() const {return traits_type::eof();}

	// Overridables
	inline int_type get();
	inline Impl_T& read(Char_T* p, const size_t nCount);
	inline bool good() const;
	inline PosType tellg() const;
	inline void seekg(const PosType& pos);
};

template <class Impl_T, class Char_T>
inline auto DFG_CLASS_NAME(BasicIStreamCRTP)<Impl_T, Char_T>::get() -> int_type
{
	return static_cast<Impl_T&>(*this).get();
}

template <class Impl_T, class Char_T>
inline Impl_T& DFG_CLASS_NAME(BasicIStreamCRTP)<Impl_T, Char_T>::read(Char_T* p, const size_t nCount)
{
	return static_cast<Impl_T&>(*this).read(p, nCount);
}

template <class Impl_T, class Char_T>
inline bool DFG_CLASS_NAME(BasicIStreamCRTP)<Impl_T, Char_T>::good() const
{
	return static_cast<Impl_T&>(*this).good();
}

template <class Impl_T, class Char_T>
inline auto DFG_CLASS_NAME(BasicIStreamCRTP)<Impl_T, Char_T>::tellg() const -> PosType
{
	return static_cast<Impl_T&>(*this).tellg();
}

template <class Impl_T, class Char_T>
inline void DFG_CLASS_NAME(BasicIStreamCRTP)<Impl_T, Char_T>::seekg(const PosType& pos)
{
	static_cast<Impl_T&>(*this).seekg(pos);
}

}} // module io

