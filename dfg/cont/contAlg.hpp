#pragma once

#include "../dfgDefs.hpp"
#include "../reflection/hasMemberFunction.hpp"
#include "../dfgBase.hpp"
#include <type_traits>
#include <map>

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

// Element-wise erase
// For now restricted to maps as element-wise erase wouldn't be optimal for storages like std::vector.
template <class T0, class T1, class Pred_T>
void eraseIf(std::map<T0, T1>& cont, Pred_T&& pred)
{
    for (auto iter = std::begin(cont), iterEnd = std::end(cont); iter != iterEnd;)
    {
        if (pred(*iter))
            iter = cont.erase(iter);
        else
            ++iter;
    }
}

// Returns true iff two ranges have identical size and elements at corresponding indexes are identical in the sense of given predicate
// If ranges are empty, returns true.
// Note: Implementation is not optimal for ranges where size() is not O(1).
// Related code: std::equal(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2); (C++14)
template <class Iterable0_T, class Iterable1_T, class Pred_T>
bool isEqualContent(const Iterable0_T& left, const Iterable1_T& right, Pred_T&& pred)
{
    const auto nSize = count(left);
    const auto nSize2 = count(right);
    if (nSize != nSize2)
        return false;
    const auto iterEnd = std::end(left);
    auto iterRight = std::begin(right);
    for (auto iter = std::begin(left); !isAtEnd(iter, iterEnd); ++iter, ++iterRight)
    {
        if (!pred(*iter, *iterRight))
            return false;
    }
    return true;
}

// Convenience overload using == for comparison
template <class Iterable0_T, class Iterable1_T>
bool isEqualContent(const Iterable0_T& left, const Iterable1_T& right)
{
    return isEqualContent(left, right, [](const decltype(*std::begin(left))& a, const decltype(*std::begin(right))& b) { return a == b; });
}

} } // module namespace
