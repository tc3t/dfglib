#pragma once

#include "../dfgDefs.hpp"
#include <string>
#include <ostream>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(str) {

// Helper class for formatting (more) human-readable representation of byte count using metric prefixes
//      -Uses a maximum precision of 4 digits, e.g. 111222 B -> "111.2 kB"
//      -Uses floor rounding: e.g. 123499 B -> "123.4 kB"
class ByteCountFormatter_metric
{
public:
    using CountT = uint64;

    ByteCountFormatter_metric() = default;

    constexpr ByteCountFormatter_metric(CountT nCount);

    static constexpr ByteCountFormatter_metric privFromRawMembers(
        const uint16 nIntPart,
        const uint16 nFracPart,
        const uint8 nExponent,
        const uint8 nFracPartLength)
    {
        ByteCountFormatter_metric bcf;
        bcf.m_nIntegerPart = nIntPart;
        bcf.m_nFractionalPart = nFracPart;
        bcf.m_nExponent = nExponent;
        bcf.m_nFracPartLength = nFracPartLength;
        return bcf;
    }

    constexpr bool operator==(const ByteCountFormatter_metric& other) const
    {
        return
            this->m_nExponent       == other.m_nExponent &&
            this->m_nFracPartLength == other.m_nFracPartLength &&
            this->m_nFractionalPart == other.m_nFractionalPart &&
            this->m_nIntegerPart    == other.m_nIntegerPart;
    }

    static constexpr char exponentPrefixChar(const uint8 nExponent)
    {
        /*
        https://en.wikipedia.org/wiki/Unit_prefix
        1000^1 	10^3 	k 	kilo
        1000^2 	10^6 	M 	mega
        1000^3 	10^9 	G 	giga
        1000^4 	10^12 	T 	tera
        1000^5 	10^15 	P 	peta
        1000^6 	10^18 	E 	exa
        */
        switch (nExponent)
        {
            case 18: return 'E';
            case 15: return 'P';
            case 12: return 'T';
            case  9: return 'G';
            case  6: return 'M';
            case  3: return 'k';
            default: return '\0';
        }
    }

    // Creates string representation of 'this'.
    std::string toString() const
    {
        // A bit cumbersome implementation in order to avoid dependency to string formatting includes.
        const char szPrefix[2] = { exponentPrefixChar(this->m_nExponent), '\0' };
        std::string s;
        s += std::to_string(this->m_nIntegerPart);
        if (this->m_nExponent >= 3 && this->m_nFracPartLength > 0)
        {
            const auto sFracInt = std::to_string(this->m_nFractionalPart);
            s += ".";
            if (this->m_nFracPartLength > sFracInt.size())
                s.insert(s.end(), this->m_nFracPartLength - sFracInt.size(), '0');
            s += sFracInt;
        }
        s += " ";
        s += szPrefix;
        s += "B";
        return s;
    }

    uint16 m_nIntegerPart    = 0;
    uint16 m_nFractionalPart = 0;
    uint8 m_nExponent        = 0;
    uint8 m_nFracPartLength  = 0;

}; // class ByteCountFormatter_metric

constexpr ByteCountFormatter_metric::ByteCountFormatter_metric(const CountT nCount)
{
    uint16 nIntegerPart = 0;
    uint16 nFractionalPart = 0;
    uint8 nExponent = 0;
    uint8 nFracPartLength = 0;
    const int nMaxDigits = 4; // This could be customisable at some point, but if using more than 5,
                              // would at least need to promote m_nFractionalPart to a bigger integer.
                              // With powers of 1000 scheme, nMaxDigits can't be less than 3.
    CountT nDivider = 1;
    if (nCount >= uint64(1000000000000000000)) // 10^18
    {
        nExponent = 18;
        nDivider = uint64(1000000000000000000);
    }
    else if (nCount >= uint64(1000000000000000)) // 10^15
    {
        nExponent = 15;
        nDivider = uint64(1000000000000000);
    }
    else if (nCount >= uint64(1000000000000)) // 10^12
    {
        nExponent = 12;
        nDivider = uint64(1000000000000);
    }
    else if (nCount >= uint64(1000000000)) // 10^9
    {
        nExponent = 9;
        nDivider = uint64(1000000000);
    }
    else if (nCount >= uint64(1000000)) // 10^6
    {
        nExponent = 6;
        nDivider = uint64(1000000);
    }
    else if (nCount >= uint64(1000)) // 10^3
    {
        nExponent = 3;
        nDivider = uint64(1000);
    }
    nIntegerPart = static_cast<decltype(nIntegerPart)>(nCount / nDivider);
    int nIntegerDigits = 0;
    if (nIntegerPart < 10)
        nIntegerDigits = 1;
    else if (nIntegerPart < 100)
        nIntegerDigits = 2;
    else
        nIntegerDigits = 3;

    const auto nMaxFracPartDigits = nMaxDigits - nIntegerDigits;
    CountT nRemainder = nCount - CountT(nIntegerPart) * CountT(nDivider);
    for (int i = 0; i < nMaxFracPartDigits && nRemainder > 0; ++i)
    {
        nDivider /= 10;
        const auto nLeadDigit = nRemainder / nDivider;
        nFractionalPart = static_cast<decltype(nFractionalPart)>(nFractionalPart * 10 + nLeadDigit);
        ++nFracPartLength;
        nRemainder -= nLeadDigit * nDivider;
    }
    if (nFractionalPart == 0)
        nFracPartLength = 0;
    *this = privFromRawMembers(nIntegerPart, nFractionalPart, nExponent, nFracPartLength);
}

} } // namespace dfg::str

inline std::ostream& operator<<(std::ostream& ostrm, const ::DFG_MODULE_NS(str)::ByteCountFormatter_metric bcfm)
{
    ostrm << bcfm.toString();
    return ostrm;
}
