#ifndef DFG_ITER_VTYFC1FL
#define DFG_ITER_VTYFC1FL

#include "iter/iteratorTemplate.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(iter) {

template <class Iter_T, class Pred_T>
Iter_T advanceWhile(Iter_T begin, const Iter_T end, Pred_T pred)
{
    for(; begin != end && pred(*begin); ++begin)
        {	}
    return begin;
}

}} // module iter

#endif // include guard
