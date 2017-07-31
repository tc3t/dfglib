#pragma once

#include "../dfgDefs.hpp"
#include "../reflection/hasMemberFunction.hpp"
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

namespace DFG_DETAIL_NS
{
    namespace cont_contAlg_hpp
    {
        DFG_REFLECTION_GENERATE_HAS_MEMBER_FUNCTION_TESTER(pop_front)
    }

    template <class Cont_T>
    void popFront(Cont_T& c, std::true_type)
    {
        c.pop_front();
    }

    template <class Cont_T>
    void popFront(Cont_T& c, std::false_type)
    {
        auto endIter = c.begin();
        ++endIter;
        c.erase(c.begin(), endIter);
    }
}

// Calls c.pop_front() or if c does not have pop_front(), uses c.erase().
// Precondition: !c.empty()
template <class Cont_T>
void popFront(Cont_T& c)
{
    DFG_DETAIL_NS::popFront(c, std::integral_constant<bool, DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<Cont_T>::value>());
}

} } // module namespace
