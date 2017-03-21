#pragma once

#include "../dfgDefs.hpp"
#include "../build/languageFeatureInfo.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    // A pair that is guaranteed to have property is_trivially_copyable == true if both template types have the same property.
    template <class T0, class T1>
    class DFG_CLASS_NAME(TrivialPair)
    {
    public:
        typedef T0 first_type;
        typedef T1 second_type;

        DFG_CLASS_NAME(TrivialPair)() {}

        DFG_CLASS_NAME(TrivialPair)(const T0& t0, const T1& t1) :
            first(t0),
            second(t1)
        {}

        DFG_CLASS_NAME(TrivialPair)(T0&& t0, T1&& t1) :
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
