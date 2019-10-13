#pragma once

#include "../dfgDefs.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "../preprocessor/compilerInfoMsvc.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    // A pair with the following properties
    //  -is_trivially_copyable == true if both template types have the same property.
    //  -Default constructor does default initialization for members (e.g. TrivialPair<int, int> a; Both a.first and a.second have indeterminate value)
    //      -NOTE: if compiler does not support '= default' or value initialization is otherwise broken (possibly VC2013), default constructor does value initialization (notably MSVC before VC2015)
    template <class T0, class T1>
    class DFG_CLASS_NAME(TrivialPair)
    {
    public:
        typedef T0 first_type;
        typedef T1 second_type;

#if DFG_LANGFEAT_HAS_DEFAULTED_AND_DELETED_FUNCTIONS && (!defined(_MSC_VER) || _MSC_VER >= DFG_MSVC_VER_2015)
        DFG_CLASS_NAME(TrivialPair)() = default;
#else // Case: compiler has no defaulting, just always value initialize.
        DFG_CLASS_NAME(TrivialPair)() DFG_NOEXCEPT(std::is_nothrow_default_constructible<first_type>::value && std::is_nothrow_default_constructible<second_type>::value) :
            first(),
            second()
        {}
#endif
        DFG_CLASS_NAME(TrivialPair)(const T0& t0, const T1& t1) DFG_NOEXCEPT(std::is_nothrow_copy_constructible<first_type>::value && std::is_nothrow_copy_constructible<second_type>::value) :
            first(t0),
            second(t1)
        {}

        DFG_CLASS_NAME(TrivialPair)(T0&& t0, T1&& t1) DFG_NOEXCEPT(std::is_nothrow_move_constructible<first_type>::value && std::is_nothrow_move_constructible<second_type>::value) :
            first(std::move(t0)),
            second(std::move(t1))
        {}

        bool operator==(const DFG_CLASS_NAME(TrivialPair)&) const;

#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
        DFG_CLASS_NAME(TrivialPair)& operator=(const DFG_CLASS_NAME(TrivialPair)& other);
        DFG_CLASS_NAME(TrivialPair)& operator=(DFG_CLASS_NAME(TrivialPair)&& other);
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

        T0 first;
        T1 second;
    }; // Class TrivialPair

    template <class T0, class T1>
    bool DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator==(const DFG_CLASS_NAME(TrivialPair)& other) const
    {
        return first == other.first && second == other.second;
    }

#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
    template <class T0, class T1>
    auto DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator=(const DFG_CLASS_NAME(TrivialPair)& other) -> DFG_CLASS_NAME(TrivialPair)&
    {
        first = other.first;
        second = other.second;
        return *this;
    }

    template <class T0, class T1>
    auto DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator=(DFG_CLASS_NAME(TrivialPair)&& other) -> DFG_CLASS_NAME(TrivialPair)&
    {
        first = std::move(other.first);
        second = std::move(other.second);
        return *this;
    }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

} } // module namespace
