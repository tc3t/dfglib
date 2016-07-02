#pragma once

#include "dfgDefs.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Wrappers for [const] char* to aid in writing code and interfaces that actually know what the content of null terminated char* means.

DFG_ROOT_NS_BEGIN
{
    enum SzPtrType
    {
        SzPtrTypeAscii, // Ascii is valid UTF8 and valid Latin-1.

        // Supersets of ASCII
        SzPtrTypeLatin1,
        SzPtrTypeUtf8,

    // Note: if adding items, see constructor of SzPtrT.
};

template <class Char_T, SzPtrType Type_T>
struct SzPtrT
{
    DFG_STATIC_ASSERT(sizeof(Char_T) == 1, "Char_T must be either char or const char");

    explicit SzPtrT(Char_T* psz) :
        m_psz(psz)
    {}

    // With nothing but ASCII and it's supersets, everything can be constructed from SzPtrAscii.
    template <class Char_T2>
    SzPtrT(const SzPtrT<Char_T2, SzPtrTypeAscii>& other) :
        m_psz(other.m_psz)
    {
    }

    // Automatic conversion from char -> const char
    operator SzPtrT<const Char_T, Type_T>() const { return SzPtrT<const Char_T, Type_T>(m_psz); }

    Char_T* c_str() const { return m_psz; }

    Char_T* m_psz;
};

/*
Define the following items for each (using Ascii as example):
typedef: SzPtrAsciiW; // Non-const ptr
typedef: SzPtrAsciiR; // Const ptr
function: SzPtrAsciiW SzPtrAscii(char* psz) // Convenience function for creating SzPtrAsciiW from non-const pointer.
function: SzPtrAsciiR SzPtrAscii(const char* psz) // Convenience function for creating SzPtrAsciiR from const pointer.
*/

#define DFG_TEMP_MACRO_CREATE_SPECIALIZATION(TYPE) \
    typedef SzPtrT<char, SzPtrType##TYPE> SzPtr##TYPE##W; \
    typedef SzPtrT<const char, SzPtrType##TYPE> SzPtr##TYPE##R; \
    inline SzPtrT<char, SzPtrType##TYPE> SzPtr##TYPE(char* psz) { return SzPtrT<char, SzPtrType##TYPE>(psz); } \
    inline SzPtrT<const char, SzPtrType##TYPE> SzPtr##TYPE(const char* psz) { return SzPtrT<const char, SzPtrType##TYPE>(psz); }

DFG_TEMP_MACRO_CREATE_SPECIALIZATION(Ascii);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(Latin1);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(Utf8);

#undef DFG_TEMP_MACRO_CREATE_SPECIALIZATION

} // root namespace
