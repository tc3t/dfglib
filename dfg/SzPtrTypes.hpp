#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "build/languageFeatureInfo.hpp"
#include <cstddef> // For std::nullptr_t

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Wrappers for [const] char* to aid in writing code and interfaces that actually know what the content of null terminated char* means.

DFG_ROOT_NS_BEGIN
{

enum CharPtrType
{
    // Char ptr types without encoding. (must have value < 0)
    CharPtrTypeChar     = -1,
    
    // Char ptr's with encoding
    CharPtrTypeAscii    = 0, // Ascii is valid UTF8 and valid Latin-1.

    // Supersets of ASCII
    CharPtrTypeLatin1,
    CharPtrTypeUtf8,

    // Note: if adding items:
    //  -See constructor of TypedCharPtrT.
    //  -Adjust gnNumberOfCharPtrTypes and gnNumberOfCharPtrTypesWithEncoding.
};

namespace DFG_DETAIL_NS
{
    const auto gnNumberOfCharPtrTypes = 4;
    const auto gnNumberOfCharPtrTypesWithEncoding = 3;

    template <CharPtrType PtrType, class Int_T>
    struct CodePointT
    {
        typedef Int_T IntegerType;

        CodePointT()
        {
        }

        explicit CodePointT(const IntegerType c) :
            m_cp(c)
        {
        }

        bool operator==(const CodePointT& other) const
        {
            return m_cp == other.m_cp;
        }

        IntegerType toInt() const { return m_cp; }

        IntegerType m_cp;
    };
} // DFG_DETAIL_NS

typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeChar, char>       CodePointChar;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeAscii, char>      CodePointAscii;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeLatin1, uint8>    CodePointLatin1;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeUtf8, uint32>     CodePointUtf8;
DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypes == 4, "");

namespace DFG_DETAIL_NS
{
    template <CharPtrType PtrType> struct TypedCharPtrDeRefType;
    template <> struct TypedCharPtrDeRefType<CharPtrTypeChar>   { typedef CodePointChar   type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeAscii>  { typedef CodePointAscii  type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeLatin1> { typedef CodePointLatin1 type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeUtf8>   { typedef CodePointUtf8   type; };
    DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypes == 4, "");
}

namespace DFG_DETAIL_NS
{
    template <CharPtrType PtrType, bool HasTrivialIndexing_T>
    struct CharPtrTypeTraitsImpl
    {
        typedef typename DFG_DETAIL_NS::TypedCharPtrDeRefType<PtrType>::type    CodePointType;
        typedef typename CodePointType::IntegerType                             CodePointInteger;

        // hasTrivialIndexing is true iff ptr type is such that operator[] can be implemented:
        //  -either every base char is a code point (e.g. latin1) or there's no (compile time) encoding.
        enum { hasTrivialIndexing = HasTrivialIndexing_T };
    };
} // namespace DFG_DETAIL_NS

template <CharPtrType PtrType_T> struct CharPtrTypeTraits {};
template <> struct CharPtrTypeTraits<CharPtrTypeChar>   : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeChar, true> {};
template <> struct CharPtrTypeTraits<CharPtrTypeAscii>  : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeAscii,  true>  {};
template <> struct CharPtrTypeTraits<CharPtrTypeLatin1> : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeLatin1, true>  {};
template <> struct CharPtrTypeTraits<CharPtrTypeUtf8>   : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeUtf8,   false> {};
DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypes == 4, "");

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

    // At moment with nothing but ASCII and it's supersets (and raw char ptr), everything can be constructed from CharPtrTypeAscii.
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

    bool operator==(const std::nullptr_t&) const { return m_p == nullptr; }
    bool operator!=(const std::nullptr_t&) const { return m_p != nullptr; }

    DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED operator bool() const
    {
        return (m_p != nullptr);
    }

    template <CharPtrType DeRef_T>
    typename std::enable_if<CharPtrTypeTraits<DeRef_T>::hasTrivialIndexing, typename CharPtrTypeTraits<DeRef_T>::CodePointType>::type privDeRefImpl() const
    {
        typedef typename CharPtrTypeTraits<DeRef_T>::CodePointType CodePointType;
        DFG_ASSERT_UB(m_p != nullptr);
        return CodePointType(*m_p);
    }

    template <CharPtrType U_T>
    typename std::enable_if<CharPtrTypeTraits<U_T>::hasTrivialIndexing, void>::type privPreIncrementImpl()
    {
        DFG_ASSERT(m_p != nullptr);
        ++m_p;
    }

    // For types with trivial indexing, enable operator*.
    // For now ignore other encodings such as UTF8 since meaning can be considered ambiguous: should it return base char or code point? If code point, operator* would need to have access to
    // encoding's parsing details since it might need to read several base chars.
    // Precondition: Underlying pointer must be dereferenceable.
    const typename CharPtrTypeTraits<Type_T>::CodePointType operator*() const
    {
        // Use static assert for more understandable compiler error, compilation will fail without as well.
        DFG_STATIC_ASSERT(CharPtrTypeTraits<Type_T>::hasTrivialIndexing, "TypedCharPtrT::operator* is implemented only for types that have trivial indexing.");
        return privDeRefImpl<Type_T>();
    }

    // For types with trivial indexing, enable operator++.
    void operator++()
    {
        // Use static assert for more understandable compiler error, compilation will fail without as well.
        DFG_STATIC_ASSERT(CharPtrTypeTraits<Type_T>::hasTrivialIndexing, "TypedCharPtrT::operator++ is implemented only for types that have trivial indexing.");
        privPreIncrementImpl<Type_T>();
    }

    template <CharPtrType U_T>
    typename std::enable_if<CharPtrTypeTraits<U_T>::hasTrivialIndexing, TypedCharPtrT>::type privOperatorPlus(const ptrdiff_t diff) const
    {
        DFG_ASSERT(m_p != nullptr);
        return TypedCharPtrT(m_p + diff);
    }

    // For types with trivial indexing, enable operator-.
    TypedCharPtrT operator+(const ptrdiff_t diff) const
    {
        // Use static assert for more understandable compiler error, compilation will fail without as well.
        DFG_STATIC_ASSERT(CharPtrTypeTraits<Type_T>::hasTrivialIndexing, "TypedCharPtrT::operator+ is implemented only for types that have trivial indexing.");
        return privOperatorPlus<Type_T>(diff);
    }

    template <CharPtrType U_T>
    typename std::enable_if<CharPtrTypeTraits<U_T>::hasTrivialIndexing, TypedCharPtrT>::type privOperatorMinus(const ptrdiff_t diff) const
    {
        DFG_ASSERT(m_p != nullptr);
        return TypedCharPtrT(m_p - diff);
    }

    // For types with trivial indexing, enable operator-.
    TypedCharPtrT operator-(const ptrdiff_t diff) const
    {
        // Use static assert for more understandable compiler error, compilation will fail without as well.
        DFG_STATIC_ASSERT(CharPtrTypeTraits<Type_T>::hasTrivialIndexing, "TypedCharPtrT::operator- is implemented only for types that have trivial indexing.");
        return privOperatorMinus<Type_T>(diff);
    }

    template <CharPtrType U_T>
    typename std::enable_if<CharPtrTypeTraits<U_T>::hasTrivialIndexing, ptrdiff_t>::type privOperatorMinusForPtrs(const TypedCharPtrT other) const
    {
        DFG_ASSERT(m_p != nullptr && other.m_p != nullptr);
        return m_p - other.m_p;
    }

    // For types with trivial indexing, enable operator-.
    ptrdiff_t operator-(const TypedCharPtrT other) const
    {
        // Use static assert for more understandable compiler error, compilation will fail without as well.
        DFG_STATIC_ASSERT(CharPtrTypeTraits<Type_T>::hasTrivialIndexing, "TypedCharPtrT::operator- is implemented only for types that have trivial indexing.");
        return privOperatorMinusForPtrs<Type_T>(other);
    }

    // Note: Char_T may be 'const char'. Allow const to return char* as constness 
    //      for this class is determined by the address pointed to, not by it's content.
    Char_T*         rawPtr() const     { return m_p; }

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
DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypesWithEncoding == 3, "Missing a specialization for char ptr type?");

#undef DFG_TEMP_MACRO_CREATE_SPECIALIZATION

// Literal creation macros.
// Usage: DFG_ASCII("abc")
// Note that DFG_ASCII("abc") and SzPtrAscii("abc") are different: the macro version is intended to guarantee that the string representation is really ASCII, while the 
// latter requires that the compiler implements "abc" as ASCII. So essentially DFG_UTF8("abc") should be equivalent to SzPtrUtf8(u8"abc")
#if DFG_LANGFEAT_UNICODE_STRING_LITERALS
    #define DFG_ASCII(x)    ::DFG_ROOT_NS::SzPtrAscii(u8##x) // TODO: verify that x is ASCII-compatible.
    #define DFG_UTF8(x)     ::DFG_ROOT_NS::SzPtrUtf8(u8##x)
#else
    #define DFG_ASCII(x)    ::DFG_ROOT_NS::SzPtrAscii(x)   // Creates typed ascii-string literal from string literal. Usage: DFG_ASCII("abc")  (TODO: implement, currently a placeholder)
    #define DFG_UTF8(x)     ::DFG_ROOT_NS::SzPtrUtf8(x)    // Creates typed utf8-string literal from string literal. Usage: DFG_UTF8("abc")    (TODO: implement, currently a placeholder)
#endif

#if DFG_LANGFEAT_U8_CHAR_LITERALS
    #define DFG_U8_CHAR(x)    u8##x
#else
    #define DFG_U8_CHAR(x)    x
#endif

} // root namespace
