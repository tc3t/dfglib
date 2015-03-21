#pragma once

#include "../dfgDefs.hpp"
#include <iterator>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(iter) {

template <class Data_T, class CategoryTag_T>
struct DFG_CLASS_NAME(IteratorTemplate) : public std::iterator<CategoryTag_T, Data_T>
{
};

}} // module iter
