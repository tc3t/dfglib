#pragma once

#include "../dfgDefs.hpp"
#include <iterator>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(iter) {

template <class Data_T, class CategoryTag_T>
struct DFG_CLASS_NAME(IteratorTemplate)
{
    typedef CategoryTag_T   iterator_category;
    typedef Data_T          value_type;
    typedef ptrdiff_t       difference_type;
    typedef Data_T*         pointer;
    typedef Data_T&         reference;
};

}} // module iter
