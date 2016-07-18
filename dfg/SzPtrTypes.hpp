#pragma once

#include "dfgDefs.hpp"
#include "build/languageFeatureInfo.hpp"
#include <cstddef> // For std::nullptr_t

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Wrappers for [const] char* to aid in writing code and interfaces that actually know what the content of null terminated char* means.

DFG_ROOT_NS_BEGIN
{
    enum CharPtrType
    {
        CharPtrTypeAscii, // Ascii is valid UTF8 and valid Latin-1.

        // Supersets of ASCII
        CharPtrTypeLatin1,
        CharPtrTypeUtf8,

    // Note: if adding items, see constructor of TypedCharPtrT.
};

template <class Char_T, CharPtrType Type_T>
struct TypedCharPtrT
{
    DFG_STATIC_ASSERT(sizeof(Char_T) == 1, "Char_T must be either char or const char");

    explicit TypedCharPtrT(std::nullptr_t) :
        m_p(nullptr)
    {}

    explicit TypedCharPtrT(Char_T* p) :
        m_p(p)
    {}

    // At moment with nothing but ASCII and it's supersets, everything can be constructed from CharPtrTypeAscii.
    template <class Char_T2>
    TypedCharPtrT(const TypedCharPtrT<Char_T2, CharPtrTypeAscii>& other) :
        m_p(other.m_p)
    {
    }

    // Automatic conversion from char -> const char
    operator TypedCharPtrT<const Char_T, Type_T>() const { return TypedCharPtrT<const Char_T, Type_T>(m_p); }

    bool operator==(const TypedCharPtrT& other) const
    {
        return m_p == other.m_p;
    }

    bool operator==(const nullptr_t&) const { return m_p == nullptr; }
    bool operator!=(const nullptr_t&) const { return m_p != nullptr; }

    DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED operator bool() const
    {
        return (m_p != nullptr);
    }

    const Char_T* rawPtr() const { return m_p; }

    Char_T* m_p;
};

template <class Char_T, CharPtrType Type_T>
struct SzPtrT : public TypedCharPtrT<Char_T, Type_T>
{
    typedef TypedCharPtrT<Char_T, Type_T> BaseClass;

    explicit SzPtrT(std::nullptr_t) :
        BaseClass(nullptr)
    {}

    explicit SzPtrT(Char_T* psz) :
        BaseClass(psz)
    {}

    // At moment with nothing but ASCII and it's supersets, everything can be constructed from CharPtrTypeAscii.
    template <class Char_T2>
    SzPtrT(const SzPtrT<Char_T2, CharPtrTypeAscii>& other) : 
        BaseClass(other)
    {
    }

    // Automatic conversion from char -> const char
    operator SzPtrT<const Char_T, Type_T>() const { return SzPtrT<const Char_T, Type_T>(this->m_p); }

    Char_T* c_str() const { return this->m_p; }
};

/*
Define the following items for each (using Ascii as example):
typedef: TypedCharPtrAsciiW; \
typedef: TypedCharPtrAsciiR; \
typedef: SzPtrAsciiW; // Non-const ptr
typedef: SzPtrAsciiR; // Const ptr
function: SzPtrAsciiW SzPtrAscii(char* psz) // Convenience function for creating SzPtrAsciiW from non-const pointer.
function: SzPtrAsciiR SzPtrAscii(const char* psz) // Convenience function for creating SzPtrAsciiR from const pointer.
function: SzPtrAsciiR SzPtrAscii(std::nullptr_t psz) // Convenience function for creating SzPtrAsciiR from nullptr.
*/

#define DFG_TEMP_MACRO_CREATE_SPECIALIZATION(TYPE) \
    typedef TypedCharPtrT<char, CharPtrType##TYPE> TypedCharPtr##TYPE##W; \
    typedef TypedCharPtrT<const char, CharPtrType##TYPE> TypedCharPtr##TYPE##R; \
    typedef SzPtrT<char, CharPtrType##TYPE> SzPtr##TYPE##W; \
    typedef SzPtrT<const char, CharPtrType##TYPE> SzPtr##TYPE##R; \
    inline SzPtrT<char, CharPtrType##TYPE> SzPtr##TYPE(char* psz) { return SzPtrT<char, CharPtrType##TYPE>(psz); } \
    inline SzPtrT<const char, CharPtrType##TYPE> SzPtr##TYPE(const char* psz) { return SzPtrT<const char, CharPtrType##TYPE>(psz); } \
    inline SzPtrT<const char, CharPtrType##TYPE> SzPtr##TYPE(std::nullptr_t) { return SzPtrT<const char, CharPtrType##TYPE>(nullptr); }

DFG_TEMP_MACRO_CREATE_SPECIALIZATION(Ascii);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(Latin1);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(Utf8);

#undef DFG_TEMP_MACRO_CREATE_SPECIALIZATION

} // root namespace
