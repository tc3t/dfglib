#pragma once

#include "../../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) { namespace DFG_DETAIL_NS {

template <class Key_T>
class KeyContainerBase
{
public:
    typedef Key_T key_type;

    KeyContainerBase() :
        m_bSorted(true)
    {}

    key_type&&  keyParamToInsertable(key_type&& key)        { return std::move(key); }

    template <class T>
    key_type    keyParamToInsertable(const T& keyParam)     { return Key_T(keyParam); }


    bool isSorted() const                                   { return m_bSorted; }

    bool m_bSorted;

}; // class KeyContainerBase

} } }
