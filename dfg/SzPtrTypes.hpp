#pragma once

#include "dfgDefs.hpp"
#include "dfgBaseTypedefs.hpp"
#include "build/languageFeatureInfo.hpp"
#include "dfgAssert.hpp"
#include <cstddef> // For std::nullptr_t
#include <type_traits>
#include "build/utils.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Wrappers for [const] char* to aid in writing code and interfaces that actually know what the content of null terminated char* means.

DFG_ROOT_NS_BEGIN
{

// Typed string enum. Note that typing does not imply validity: e.g. having a variable of type SzPtrUtf8R does not guarantee that the content is valid UTF-8.
//      Rationale: having validity guarantee would be useful in many places, but in practice this might bring notable overhead (e.g. in case of TableCsv)
//                 and would need specification how code like    auto a = SzPtrUtf8("some invalid UTF-8 here")    would behave.
// TODO: investigate this further; e.g. should there be an additional type/enum that explicitly promises content to be valid?
enum CharPtrType
{
    // Char ptr types without encoding. (must have value < 0).
    CharPtrTypeChar32   = -4,
    CharPtrTypeChar16   = -3,
    CharPtrTypeCharW    = -2,
    CharPtrTypeCharC    = -1,
    
    // Char ptr's with encoding
    CharPtrTypeAscii    = 0, // Ascii is valid UTF8 and valid Latin-1.

    // Supersets of ASCII
    CharPtrTypeLatin1,
    CharPtrTypeUtf8, // BaseChar = char, code point = uint32

    CharPtrTypeUtf16 // BaseChar = char16_t, code point = uint32

    // Note: if adding items:
    //  -See constructor of TypedCharPtrT.
    //  -Adjust gnNumberOfCharPtrTypes and gnNumberOfCharPtrTypesWithEncoding.
};

namespace DFG_DETAIL_NS
{
    template <CharPtrType TypeEnum_T> using CharPtrTypeEnumType = std::integral_constant<int, TypeEnum_T>;
}

template <CharPtrType TypeEnum_T> struct CharPtrTypeToBaseCharType                    { DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(DFG_DETAIL_NS::CharPtrTypeEnumType<TypeEnum_T>, "CharPtrTypeToBaseCharType specialization is missing"); };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeCharC>  { using type = char; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeCharW>  { using type = wchar_t; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeChar16> { using type = char16_t; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeChar32> { using type = char32_t; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeAscii>  { using type = char; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeLatin1> { using type = char; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeUtf8>   { using type = char; };
template <>                       struct CharPtrTypeToBaseCharType<CharPtrTypeUtf16>  { using type = char16_t; };

namespace DFG_DETAIL_NS
{
    const auto gnNumberOfCharPtrTypes = 8;
    const auto gnNumberOfCharPtrTypesWithEncoding = 4;

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

typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeCharC,  char>     CodePointCharC;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeCharW,  wchar_t>  CodePointCharW;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeChar16, char16_t> CodePointChar16;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeChar32, char32_t> CodePointChar32;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeAscii,  char>     CodePointAscii;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeLatin1, uint8>    CodePointLatin1;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeUtf8,   uint32>   CodePointUtf8;
typedef DFG_DETAIL_NS::CodePointT<CharPtrTypeUtf16,  uint32>   CodePointUtf16;
DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypes == 8, "");

namespace DFG_DETAIL_NS
{
    template <CharPtrType PtrType> struct TypedCharPtrDeRefType;
    template <> struct TypedCharPtrDeRefType<CharPtrTypeCharC>  { typedef CodePointCharC  type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeCharW>  { typedef CodePointCharW  type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeChar16> { typedef CodePointChar16 type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeChar32> { typedef CodePointChar32 type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeAscii>  { typedef CodePointAscii  type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeLatin1> { typedef CodePointLatin1 type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeUtf8>   { typedef CodePointUtf8   type; };
    template <> struct TypedCharPtrDeRefType<CharPtrTypeUtf16>  { typedef CodePointUtf16  type; };
    DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypes == 8, "");
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
template <> struct CharPtrTypeTraits<CharPtrTypeCharC>  : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeCharC,  true> {};
template <> struct CharPtrTypeTraits<CharPtrTypeCharW>  : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeCharW,  true> {};
template <> struct CharPtrTypeTraits<CharPtrTypeChar16> : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeChar16, true> {};
template <> struct CharPtrTypeTraits<CharPtrTypeChar32> : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeChar32, true> {};
template <> struct CharPtrTypeTraits<CharPtrTypeAscii>  : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeAscii,  true>  {};
template <> struct CharPtrTypeTraits<CharPtrTypeLatin1> : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeLatin1, true>  {};
template <> struct CharPtrTypeTraits<CharPtrTypeUtf8>   : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeUtf8,   false> {};
template <> struct CharPtrTypeTraits<CharPtrTypeUtf16>  : public DFG_DETAIL_NS::CharPtrTypeTraitsImpl<CharPtrTypeUtf16,  false> {};
DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypes == 8, "");

namespace DFG_DETAIL_NS
{
    // Baseclass when requiring explicit construction from raw type, e.g. char* -> TypedCharPtrUtf8
    template <class Char_T, CharPtrType Type_T>
    class TypedCharPtrTExplicitBase
    {
    public:
        explicit TypedCharPtrTExplicitBase(std::nullptr_t) :
            m_p(nullptr)
        {}

        explicit TypedCharPtrTExplicitBase(Char_T* p) :
            m_p(p)
        {}

        template <class Char_T2>
        TypedCharPtrTExplicitBase(const TypedCharPtrTExplicitBase<Char_T2, CharPtrTypeAscii>& other) :
            m_p(other.m_p)
        {
        }

        Char_T* m_p;
    };

    // Baseclass when allowing implicit construction from raw type, e.g. char16_t* -> TypedCharPtrUtf16
    template <class Char_T, CharPtrType Type_T>
    class TypedCharPtrTImplicitBase
    {
    public:
        explicit TypedCharPtrTImplicitBase(std::nullptr_t) :
            m_p(nullptr)
        {}

        TypedCharPtrTImplicitBase(Char_T* p) :
            m_p(p)
        {}

#if defined(_WIN32)
        using WCharT = typename std::conditional<std::is_const<Char_T>::value, const wchar_t, wchar_t>::type;
        DFG_STATIC_ASSERT(sizeof(Char_T) == sizeof(WCharT), "Implementation assumes Char_T and wchar_t to have the same size");
        TypedCharPtrTImplicitBase(WCharT* p) :
            m_p(reinterpret_cast<Char_T*>(p))
        {}

        operator WCharT* () const
        {
            return reinterpret_cast<WCharT*>(m_p);
        }
#endif

        Char_T* m_p;
    };
} // namespace DFG_DETAIL_NS

template <class Char_T, CharPtrType Type_T>
struct TypedCharPtrT : public std::conditional<Type_T == CharPtrTypeUtf16 || Type_T == CharPtrTypeChar32, DFG_DETAIL_NS::TypedCharPtrTImplicitBase<Char_T, Type_T>, DFG_DETAIL_NS::TypedCharPtrTExplicitBase<Char_T, Type_T>>::type
{
    using BaseClass = typename std::conditional<Type_T == CharPtrTypeUtf16 || Type_T == CharPtrTypeChar32, DFG_DETAIL_NS::TypedCharPtrTImplicitBase<Char_T, Type_T>, DFG_DETAIL_NS::TypedCharPtrTExplicitBase<Char_T, Type_T>>::type;
    DFG_STATIC_ASSERT(sizeof(Char_T) <= 2, "Char_T must have size 1 or 2");

    // Making sure that Char_T and type defined for Type_T are the same (ignoring const difference)
    DFG_STATIC_ASSERT((std::is_same<typename std::remove_const<Char_T>::type, typename CharPtrTypeToBaseCharType<Type_T>::type>::value), "Char_T must be compatible with given type");

    using CharTConstInverse = typename std::conditional<std::is_const<Char_T>::value, typename std::remove_const<Char_T>::type, const Char_T>::type;

    using BaseClass::BaseClass; // Inheriting constructor

    // Automatic conversion from char -> const char
    operator TypedCharPtrT<const Char_T, Type_T>() const { return TypedCharPtrT<const Char_T, Type_T>(this->m_p); }

    bool operator==(const TypedCharPtrT& other) const
    {
        return this->m_p == other.m_p;
    }

    // This was added for MSVC2022 C++20 mode, dfgTest wouldn't compile without.
    bool operator==(const TypedCharPtrT<CharTConstInverse, Type_T>& other) const
    {
        return this->m_p == other.m_p;
    }

    bool operator!=(const TypedCharPtrT& other) const
    {
        return !(this->m_p == other.m_p);
    }

    bool operator==(const std::nullptr_t&) const { return this->m_p == nullptr; }
    bool operator!=(const std::nullptr_t&) const { return this->m_p != nullptr; }

    DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED operator bool() const
    {
        return (this->m_p != nullptr);
    }

    template <CharPtrType DeRef_T>
    typename std::enable_if<CharPtrTypeTraits<DeRef_T>::hasTrivialIndexing, typename CharPtrTypeTraits<DeRef_T>::CodePointType>::type privDeRefImpl() const
    {
        typedef typename CharPtrTypeTraits<DeRef_T>::CodePointType CodePointType;
        DFG_ASSERT_UB(this->m_p != nullptr);
        return CodePointType(*this->m_p);
    }

    template <CharPtrType U_T>
    typename std::enable_if<CharPtrTypeTraits<U_T>::hasTrivialIndexing, void>::type privPreIncrementImpl()
    {
        DFG_ASSERT(this->m_p != nullptr);
        ++this->m_p;
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
        DFG_ASSERT(this->m_p != nullptr || diff == 0);
        return TypedCharPtrT(this->m_p + diff);
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
        DFG_ASSERT(this->m_p != nullptr);
        return TypedCharPtrT(this->m_p - diff);
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
        DFG_ASSERT((this->m_p != nullptr && other.m_p != nullptr) || (this->m_p == other.m_p));
        return this->m_p - other.m_p;
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
    Char_T*         rawPtr() const     { return this->m_p; }
}; // TypedCharPtrT


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

    explicit SzPtrT(TypedCharPtrT<Char_T, Type_T> tpsz) :
        BaseClass(tpsz)
    {}

    // At moment with nothing but ASCII and it's supersets for char size 1, everyone of those can be constructed from CharPtrTypeAscii.
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
#define DFG_TEMP_MACRO_CREATE_SPECIALIZATION(RAWCHAR, TYPE) \
    typedef TypedCharPtrT<      RAWCHAR, CharPtrType##TYPE> TypedCharPtr##TYPE##W; \
    typedef TypedCharPtrT<const RAWCHAR, CharPtrType##TYPE> TypedCharPtr##TYPE##R; \
    typedef SzPtrT<      RAWCHAR, CharPtrType##TYPE> SzPtr##TYPE##W; \
    typedef SzPtrT<const RAWCHAR, CharPtrType##TYPE> SzPtr##TYPE##R; \
    inline SzPtr##TYPE##W SzPtr##TYPE(RAWCHAR* psz)       { return SzPtrT<RAWCHAR, CharPtrType##TYPE>(psz); } \
    inline SzPtr##TYPE##R SzPtr##TYPE(const RAWCHAR* psz) { return SzPtrT<const RAWCHAR, CharPtrType##TYPE>(psz); } \
    inline SzPtr##TYPE##R SzPtr##TYPE(std::nullptr_t)     { return SzPtrT<const RAWCHAR, CharPtrType##TYPE>(nullptr); }

DFG_TEMP_MACRO_CREATE_SPECIALIZATION(char,     Ascii);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(char,     Latin1);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(char,     Utf8);
DFG_TEMP_MACRO_CREATE_SPECIALIZATION(char16_t, Utf16);


DFG_STATIC_ASSERT(DFG_DETAIL_NS::gnNumberOfCharPtrTypesWithEncoding == 4, "Missing a specialization for char ptr type?");

#undef DFG_TEMP_MACRO_CREATE_SPECIALIZATION

// Literal creation macros.
// Usage: DFG_ASCII("abc")
// Note that DFG_ASCII("abc") and SzPtrAscii("abc") are different: the macro version is intended to guarantee that the string representation is really ASCII, while the 
// latter requires that the compiler implements "abc" as ASCII. So essentially DFG_UTF8("abc") should be equivalent to SzPtrUtf8(u8"abc") (at least in C++17)
#if DFG_LANGFEAT_UNICODE_STRING_LITERALS
    #define DFG_ASCII(x)    ::DFG_ROOT_NS::SzPtrAscii(reinterpret_cast<const char*>(u8##x)) // TODO: verify that x is ASCII-compatible.
    #define DFG_UTF8(x)     ::DFG_ROOT_NS::SzPtrUtf8(reinterpret_cast<const char*>(u8##x))
    #define DFG_UTF16(x)    ::DFG_ROOT_NS::SzPtrUtf16(u##x)
#else
    #define DFG_ASCII(x)    ::DFG_ROOT_NS::SzPtrAscii(x)   // Creates typed ascii-string literal from string literal. Usage: DFG_ASCII("abc")  (TODO: implement, currently a placeholder)
    #define DFG_UTF8(x)     ::DFG_ROOT_NS::SzPtrUtf8(x)    // Creates typed utf8-string literal from string literal. Usage: DFG_UTF8("abc")    (TODO: implement, currently a placeholder)
#endif

#if DFG_LANGFEAT_U8_CHAR_LITERALS
    #define DFG_U8_CHAR(x)    u8##x
#else
    #define DFG_U8_CHAR(x)    x
#endif

template <class Ptr_T> constexpr CharPtrType charPtrTypeByPtr();
template <> constexpr CharPtrType charPtrTypeByPtr<char*>()               { return CharPtrTypeCharC; }
template <> constexpr CharPtrType charPtrTypeByPtr<const char*>()         { return CharPtrTypeCharC; }
template <> constexpr CharPtrType charPtrTypeByPtr<wchar_t*>()            { return CharPtrTypeCharW; }
template <> constexpr CharPtrType charPtrTypeByPtr<const wchar_t*>()      { return CharPtrTypeCharW; }
template <> constexpr CharPtrType charPtrTypeByPtr<char16_t*>()           { return CharPtrTypeChar16; }
template <> constexpr CharPtrType charPtrTypeByPtr<const char16_t*>()     { return CharPtrTypeChar16; }
template <> constexpr CharPtrType charPtrTypeByPtr<const char32_t*>()     { return CharPtrTypeChar32; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrAsciiR>()  { return CharPtrTypeAscii; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrAsciiW>()  { return CharPtrTypeAscii; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrLatin1R>() { return CharPtrTypeLatin1; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrLatin1W>() { return CharPtrTypeLatin1; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrUtf8R>()   { return CharPtrTypeUtf8; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrUtf8W>()   { return CharPtrTypeUtf8; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrUtf16R>()  { return CharPtrTypeUtf16; }
template <> constexpr CharPtrType charPtrTypeByPtr<TypedCharPtrUtf16W>()  { return CharPtrTypeUtf16; }

} // root namespace
