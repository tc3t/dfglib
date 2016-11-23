#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    // A pair that is guaranteed to have property is_trivially_copyable == true if both template types have the same property.
    template <class T0, class T1>
    class DFG_CLASS_NAME(TrivialPair)
    {
    public:
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

        T0 first;
        T1 second;
    }; // Class TrivialPair

    template <class T0, class T1>
    bool DFG_CLASS_NAME(TrivialPair)<T0, T1>::operator==(const DFG_CLASS_NAME(TrivialPair)& other) const
    {
        return first == other.first && second == other.second;
    }

} } // module namespace
