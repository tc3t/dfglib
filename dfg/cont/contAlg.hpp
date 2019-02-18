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
        DFG_REFLECTION_GENERATE_HAS_MEMBER_FUNCTION_TESTER(cutTail)
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

    template <class Cont_T>
    void cutTail(Cont_T& c, const typename Cont_T::const_iterator iter,  std::true_type)
    {
        c.cutTail(iter);
    }

    template <class Cont_T>
    void cutTail(Cont_T& c, typename Cont_T::const_iterator iter, std::false_type)
    {
#if defined(__GNUG__) && (__GNUC__ < 5) // Workaround for GCC not liking const_iterator's in erase(); should be fixed for all containers by 5.0 (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60278)
        auto iterFirst = c.end();
        std::advance(iterFirst, -1 * std::distance(iter, c.cend()));
        c.erase(iterFirst, c.end());
#else
        c.erase(iter, c.cend());
#endif
        
    }
}

// Calls c.pop_front() or if c does not have pop_front(), uses c.erase().
// Precondition: !c.empty()
template <class Cont_T>
void popFront(Cont_T& c)
{
    DFG_DETAIL_NS::popFront(c, std::integral_constant<bool, DFG_DETAIL_NS::cont_contAlg_hpp::Has_pop_front<Cont_T>::value>());
}

// Removes tail from given container expecting an iterator to tail's first element (i.e. the first element to be removed).
template <class Cont_T>
void cutTail(Cont_T& c, const typename Cont_T::const_iterator iterToFirstInTail)
{
    DFG_DETAIL_NS::cutTail(c, iterToFirstInTail, std::integral_constant<bool, DFG_DETAIL_NS::cont_contAlg_hpp::Has_cutTail<Cont_T>::value>());
}

// Tries to reserve() memory for container returning true if successful, false otherwise.
template <class Cont_T>
bool tryReserve(Cont_T& cont, const size_t nCount)
{
    try
    {
        cont.reserve(nCount);
        return (cont.capacity() >= nCount);
    }
    catch (...)
    {
        return false;
    }
}

} } // module namespace
