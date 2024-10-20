#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

// Class for typesafer flag operations for given enum.
// For defining new enums that use Flags-class under the hood, see macro DFG_DEFINE_SCOPED_ENUM_FLAGS_WITH_OPERATORS
// Related implementations
//      -QFlags
template <class Enum_T>
class Flags
{
public:
    using Enum = Enum_T;
    using IntT = typename std::underlying_type<Enum>::type;
    constexpr Flags() = default;
    constexpr Flags(Enum e) : m_nFlags(static_cast<IntT>(e)) {}
    constexpr static Flags fromNumber(const IntT number) { return Flags(number); }
private:
    constexpr Flags(const IntT val) : m_nFlags(val) {}

public:
    Flags& operator|=(Enum flag)         { m_nFlags |= flag;           return *this; }
    Flags& operator|=(const Flags other) { m_nFlags |= other.m_nFlags; return *this; }
    Flags& operator&=(Enum flag)         { m_nFlags &= flag;           return *this; }
    Flags& operator&=(const Flags other) { m_nFlags &= other.m_nFlags; return *this; }
    Flags& operator^=(Enum flag)         { m_nFlags ^= flag;           return *this; }
    Flags& operator^=(const Flags other) { m_nFlags ^= other.m_nFlags; return *this; }
    constexpr Flags  operator|(const Enum flag)   const { return Flags(toNumber() | static_cast<IntT>(flag)); }
    constexpr Flags  operator|(const Flags other) const { return Flags(toNumber() | other.toNumber()); }
    constexpr Flags  operator&(const Enum flag)   const { return Flags(toNumber() & static_cast<IntT>(flag)); }
    constexpr Flags  operator&(const Flags other) const { return Flags(toNumber() & other.toNumber()); }
    constexpr Flags  operator^(const Enum flag)   const { return Flags(toNumber() ^ static_cast<IntT>(flag)); }
    constexpr Flags  operator^(const Flags other) const { return Flags(toNumber() ^ other.toNumber()); }

    constexpr bool operator==(const Flags other) const { return this->m_nFlags == other.m_nFlags; } \
    constexpr bool operator!=(const Flags other) const { return !(*this == other); } \

    Flags& setFlag(Enum flag, bool bOn = true) { if (bOn) *this |= flag; else this->m_nFlags &= ~flag; return *this; }
    constexpr bool testFlag(Enum flag) const { return (this->m_nFlags & static_cast<IntT>(flag)) != 0; }

    constexpr explicit operator bool() const { return this->m_nFlags != 0; }
    constexpr bool operator!() const { return this->m_nFlags == 0; }
    constexpr Flags operator~() const { return Flags(~this->m_nFlags); }
    constexpr IntT toNumber() const { return m_nFlags; }

    constexpr inline void operator+(Flags other) const noexcept = delete;
    constexpr inline void operator+(Enum other)  const noexcept = delete;
    constexpr inline void operator+(IntT other)  const noexcept = delete;
    constexpr inline void operator-(Flags other) const noexcept = delete;
    constexpr inline void operator-(Enum other)  const noexcept = delete;
    constexpr inline void operator-(IntT other)  const noexcept = delete;
    constexpr inline void operator*(Flags other) const noexcept = delete;
    constexpr inline void operator*(Enum other)  const noexcept = delete;
    constexpr inline void operator*(IntT other)  const noexcept = delete;

    IntT m_nFlags = 0;
}; // class Flags

///////////////////////////////
// DFG_DEFINE_SCOPED_ENUM_FLAGS
//      Defines enum class -like entity whose enum-items can be used like flags:
//          -All enum items are scoped under SCOPE_NAME
//          -Can define multiple enums e.g. in one class.
//      Known shortcomings:
//          -Comparison of different enum items compiles (e.g. Enum1::a == Enum2::a compiles, but e.g. Qt Creator 5.0.2 generated "warning: comparison of different enumeration types")
//          -Operations between different enums create complex compile errors (e.g. Enum1::a | Enum2::a)
//          -If operators for enum are not defined with DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS(), operators such as | compile with enum items as well as with enum/int mix.
// Example:
//      DFG_DEFINE_SCOPED_ENUM_FLAGS_WITH_OPERATORS(TestFlags, ::DFG_ROOT_NS::uint16,
//          flagOne = 0x1,
//          flagTwo = 0x2,
//          flagThree = 0x4,
//          flagsOneAndThree = flagOne | flagThree
//          )
//      void func()
//      {
//          const auto flags = TestFlags2::flagOne | TestFlags2::flagThree;
//      }
// Implementation note: this macro creates a wrapper class for Flags: defines enums and uses that to instantiate Flags-template class.
//                      Almost completely boilerplate.
#define DFG_DEFINE_SCOPED_ENUM_FLAGS(SCOPE_NAME, INTTYPE, ...) \
class SCOPE_NAME \
{ \
public: \
    enum Enum : INTTYPE \
    { \
        __VA_ARGS__ \
    };\
    using Storage = ::DFG_MODULE_NS(cont)::Flags<Enum>; \
\
    constexpr SCOPE_NAME() = default; \
    constexpr SCOPE_NAME(Enum e) : m_flags(e) {} \
    constexpr SCOPE_NAME(Storage flags) : m_flags(flags) { } \
    constexpr static SCOPE_NAME fromNumber(const typename Storage::IntT number) { return SCOPE_NAME(Storage::fromNumber(number)); } \
\
    SCOPE_NAME& operator|=(Enum flag)              { m_flags |= flag;          return *this; } \
    SCOPE_NAME& operator|=(const SCOPE_NAME other) { m_flags |= other.m_flags; return *this; } \
    SCOPE_NAME& operator&=(Enum flag)              { m_flags &= flag;          return *this; } \
    SCOPE_NAME& operator&=(const SCOPE_NAME other) { m_flags &= other.m_flags; return *this; } \
    SCOPE_NAME& operator^=(Enum flag)              { m_flags ^= flag;          return *this; } \
    SCOPE_NAME& operator^=(const SCOPE_NAME other) { m_flags ^= other.m_flags; return *this; } \
    constexpr SCOPE_NAME  operator|(const Enum flag)        const { return m_flags | flag; } \
    constexpr SCOPE_NAME  operator|(const SCOPE_NAME other) const { return m_flags | other.m_flags; } \
    constexpr SCOPE_NAME  operator&(const Enum flag)        const { return m_flags & flag; } \
    constexpr SCOPE_NAME  operator&(const SCOPE_NAME other) const { return m_flags & other.m_flags; } \
    constexpr SCOPE_NAME  operator^(const Enum flag)        const { return m_flags ^ flag; } \
    constexpr SCOPE_NAME  operator^(const SCOPE_NAME other) const { return m_flags ^ other.m_flags; } \
    constexpr bool operator==(const SCOPE_NAME other) const { return this->m_flags == other.m_flags; } \
    constexpr bool operator!=(const SCOPE_NAME other) const { return this->m_flags != other.m_flags; } \
\
    SCOPE_NAME& setFlag(Enum flag, bool bOn = true) { m_flags.setFlag(flag, bOn); return *this; }  \
    constexpr bool testFlag(Enum flag) const { return this->m_flags.testFlag(flag); } \
    constexpr explicit operator bool() const { return this->m_flags.operator bool(); } \
    constexpr bool operator!() const { return !this->m_flags; } \
    constexpr SCOPE_NAME operator~() const { return SCOPE_NAME(~this->m_flags); } \
    constexpr INTTYPE toNumber() const { return m_flags.toNumber(); } \
\
    constexpr inline void operator+(SCOPE_NAME other) const noexcept = delete; \
    constexpr inline void operator+(Enum other)       const noexcept = delete; \
    constexpr inline void operator+(INTTYPE other)    const noexcept = delete; \
    constexpr inline void operator-(SCOPE_NAME other) const noexcept = delete; \
    constexpr inline void operator-(Enum other)       const noexcept = delete; \
    constexpr inline void operator-(INTTYPE other)    const noexcept = delete; \
    constexpr inline void operator*(SCOPE_NAME other) const noexcept = delete; \
    constexpr inline void operator*(Enum other)       const noexcept = delete; \
    constexpr inline void operator*(INTTYPE other)    const noexcept = delete; \
\
    Storage m_flags; \
}; // SCOPE_NAME class
// End of macro-definition for DFG_DEFINE_SCOPED_ENUM_FLAGS
///////////////////////////////////////////////////////////


//////////////////////////////////////////
// DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS
//      Defines operators for given enum type created by DFG_DEFINE_SCOPED_ENUM_FLAGS-macro and deletes operations that should not be accepted.
//
#define DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS(SCOPE_NAME) \
    DFG_MAYBE_UNUSED constexpr inline SCOPE_NAME operator| (SCOPE_NAME::Enum f1, SCOPE_NAME::Enum f2) noexcept { return SCOPE_NAME::Storage(f1) | f2;} \
    template <class T> inline SCOPE_NAME operator| (SCOPE_NAME::Enum f1, T f2) noexcept = delete; \
    template <class T> inline SCOPE_NAME operator| (T f1, SCOPE_NAME::Enum f2) noexcept = delete; \
    DFG_MAYBE_UNUSED constexpr inline SCOPE_NAME operator& (SCOPE_NAME::Enum f1, SCOPE_NAME::Enum f2) noexcept { return SCOPE_NAME::Storage(f1) & f2;} \
    template <class T> inline SCOPE_NAME operator& (SCOPE_NAME::Enum f1, T f2) noexcept = delete; \
    template <class T> inline SCOPE_NAME operator& (T f1, SCOPE_NAME::Enum f2) noexcept = delete; \
    DFG_MAYBE_UNUSED constexpr inline SCOPE_NAME operator^ (SCOPE_NAME::Enum f1, SCOPE_NAME::Enum f2) noexcept { return SCOPE_NAME::Storage(f1) ^ f2;} \
    template <class T> inline SCOPE_NAME operator^ (SCOPE_NAME::Enum f1, T f2) noexcept = delete; \
    template <class T> inline SCOPE_NAME operator^ (T f1, SCOPE_NAME::Enum f2) noexcept = delete; \
    constexpr inline void operator+(SCOPE_NAME::Enum f1, SCOPE_NAME::Enum f2)      noexcept = delete; \
    constexpr inline void operator+(SCOPE_NAME::Enum f1, SCOPE_NAME f2)            noexcept = delete; \
    constexpr inline void operator+(int f1, SCOPE_NAME f2)                         noexcept = delete; \
    constexpr inline void operator+(int f1, SCOPE_NAME::Enum f2)                   noexcept = delete; \
    constexpr inline void operator+(SCOPE_NAME::Enum f1, int f2)                   noexcept = delete; \
    constexpr inline void operator-(SCOPE_NAME::Enum f1, SCOPE_NAME::Enum f2)      noexcept = delete; \
    constexpr inline void operator-(SCOPE_NAME::Enum f1, SCOPE_NAME f2)            noexcept = delete; \
    constexpr inline void operator-(int f1, SCOPE_NAME f2)                         noexcept = delete; \
    constexpr inline void operator-(int f1, SCOPE_NAME::Enum f2)                   noexcept = delete; \
    constexpr inline void operator-(SCOPE_NAME::Enum f1, int f2)                   noexcept = delete; \
    constexpr inline void operator*(SCOPE_NAME::Enum f1, SCOPE_NAME::Enum f2)      noexcept = delete; \
    constexpr inline void operator*(SCOPE_NAME::Enum f1, SCOPE_NAME f2)            noexcept = delete; \
    constexpr inline void operator*(int f1, SCOPE_NAME f2)                         noexcept = delete; \
    constexpr inline void operator*(int f1, SCOPE_NAME::Enum f2)                   noexcept = delete; \
    constexpr inline void operator*(SCOPE_NAME::Enum f1, int f2)                   noexcept = delete;
// End of macro-definition for DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS
//////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////
// DFG_DEFINE_SCOPED_ENUM_FLAGS_WITH_OPERATORS
//      Defines enum and it's operators. Note that this can't be used inside a class:
//          -For class, use DFG_DEFINE_SCOPED_ENUM_FLAGS() inside a class and for operators, DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS() outside the class.
//          -Example:
//              class FlagsTestClass
//              {
//              public:
//                  DFG_DEFINE_SCOPED_ENUM_FLAGS(Enums, int,
//                      a = 1,
//                      b = 2)
//              };
//              DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS(FlagsTestClass::Enums)
#define DFG_DEFINE_SCOPED_ENUM_FLAGS_WITH_OPERATORS(SCOPE_NAME, INTTYPE, ...) \
    DFG_DEFINE_SCOPED_ENUM_FLAGS(SCOPE_NAME, INTTYPE, __VA_ARGS__) \
    DFG_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS(SCOPE_NAME)


} } // module cont
