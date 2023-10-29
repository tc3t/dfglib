#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>
#if defined(_MSC_VER)
	#include <stdlib.h> // For _byteswap_*
#endif

#include <boost/predef/other/endian.h>

DFG_ROOT_NS_BEGIN
{

	enum ByteOrder
	{
		ByteOrderUnknown,
		ByteOrderLittleEndian,
		ByteOrderBigEndian,
#if BOOST_ENDIAN_LITTLE_BYTE
		ByteOrderHost = ByteOrderLittleEndian
#elif BOOST_ENDIAN_BIG_BYTE
		ByteOrderHost = ByteOrderBigEndian
#else
		ByteOrderHost = ByteOrderUnknown
#endif
	};

	// Returns host byte order detected (potentially) at run time.
	// TODO: test
	inline ByteOrder byteOrderHost()
	{
		return ByteOrderHost;
	}

	// TODO: Replace by non-Windows specific versions and by versions that allow compile time swapping.
#if defined(_MSC_VER)
	template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 1>) { return val; }
	template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 2>) { return _byteswap_ushort(val); }
	template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 4>) { return _byteswap_ulong(val); }
	template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 8>) { return _byteswap_uint64(val); }
#else // Implementation below works at least in GCC/MinGW.
    template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 1>) { return val; }
    template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 2>) { return __builtin_bswap16(val); }
    template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 4>) { return __builtin_bswap32(val); }
    template <class T> T byteSwapImpl(T val, std::integral_constant<size_t, 8>) { return __builtin_bswap64(val); }
#endif

	template <class T>
	inline T byteSwap(T val)
	{
		DFG_STATIC_ASSERT(std::is_integral<T>::value, "Expecting integral type");
		DFG_STATIC_ASSERT(std::is_unsigned<T>::value || sizeof(T) == 1, "Expecting unsigned type");
		return byteSwapImpl(val, std::integral_constant<size_t, sizeof(T)>());

	}

	template <class T> inline T byteSwap(T val, ByteOrder from, ByteOrder to) { return (from != to) ? byteSwap(val) : val; }
	template <> inline char byteSwap(char val, ByteOrder, ByteOrder) { return val; }
	template <> inline unsigned char byteSwap(unsigned char val, ByteOrder, ByteOrder) { return val; }

	template <class T> inline T byteSwapHostToBigEndian(T val) { return (byteOrderHost() == ByteOrderLittleEndian) ? byteSwap(val) : val; }
	template <class T> inline T byteSwapHostToLittleEndian(T val) { return (byteOrderHost() == ByteOrderBigEndian) ? byteSwap(val) : val; }
	template <class T> inline T byteSwapBigEndianToHost(T val) { return byteSwapHostToBigEndian(val); }
	template <class T> inline T byteSwapLittleEndianToHost(T val) { return byteSwapHostToLittleEndian(val); }


} // namespace
