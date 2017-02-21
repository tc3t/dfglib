#pragma once

#include "../../dfgDefs.hpp"
#include "../../alg/find.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) { namespace DFG_DETAIL_NS {

template <class Traits_T>
class KeyContainerBase
{
public:
    typedef typename Traits_T::key_type         key_type;
    typedef typename Traits_T::iterator         iterator;
    typedef typename Traits_T::const_iterator   const_iterator;

    typedef typename Traits_T::key_iterator         key_iterator;
    typedef typename Traits_T::const_key_iterator   const_key_iterator;

    template <class This_T>
    class KeyIteratorValueToComparable
    {
    public:
        KeyIteratorValueToComparable(This_T& rThis) :
            m_rThis(rThis)
        {}

        auto operator()(const typename key_iterator::value_type& keyItem) const -> const typename This_T::key_type&
        {
            return m_rThis.keyIterValueToKeyValue(keyItem);
        }

        This_T& m_rThis;
    };

    KeyContainerBase() :
        m_bSorted(true)
    {}

    key_type&&  keyParamToInsertable(key_type&& key)        { return std::move(key); }

    template <class T>
    key_type    keyParamToInsertable(const T& keyParam)     { return key_type(keyParam); }

    template <class T0, class T1>
    static bool isKeyMatch(const T0& a, const T1& b)
    {
        return DFG_MODULE_NS(alg)::isKeyMatch(a, b);
    }

    template <class This_T, class T>
    static auto findImpl(This_T& rThis, const T& key) -> decltype(rThis.makeIterator(0)) // iterator // std::conditional<std::is_const<This_T>::value, const_iterator, iterator>::type //, decltype(rThis.makeIterator(0))
    {
        if (rThis.isSorted())
        {
            auto iter = rThis.findInsertPos(rThis, key);
            return (iter != rThis.endKey() && isKeyMatch(rThis.keyIterValueToKeyValue(*iter), key)) ? rThis.makeIterator(iter - rThis.beginKey()) : rThis.end();
        }
        else
            return rThis.makeIterator(DFG_MODULE_NS(alg)::findLinear(rThis.beginKey(), rThis.endKey(), key, KeyIteratorValueToComparable<This_T>(rThis)) - rThis.beginKey());
    }

    bool isSorted() const       { return m_bSorted; }

    bool m_bSorted;

}; // class KeyContainerBase

} } }
