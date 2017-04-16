#pragma once

#include "../../dfgDefs.hpp"
#include "../../alg/find.hpp"
#include "../../ReadOnlySzParam.hpp"
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) { namespace DFG_DETAIL_NS {

/* KeySearchTypeToInternalSearchType defines the type that is used when searching keys of type 'Key_T' using search param 'Search_T'.
 By default the type is 'Search_T'. Users can add their specializations if needed for example using the following as starting point:
 namespace DFG_ROOT_NS
 {
    namespace DFG_SUB_NS_NAME(cont)
    {
        namespace DFG_DETAIL_NS
        {
            template <> struct KeySearchTypeToInternalSearchType<KeyType, SearchType> { typedef CustomSearchType type; };
        }
    }
 }
 */
template <class Key_T, class Search_T> struct KeySearchTypeToInternalSearchType     { typedef Search_T type; };

// When searching const char* from std::string's, especially with linear search there can be a massive performance difference if using const char* instead of std::string or string view, because const char*
// does not have it's length information readily available, so the comparison can't use size() != size() the determine inequality.
template <> struct KeySearchTypeToInternalSearchType<std::string, const char*>      { typedef DFG_CLASS_NAME(StringViewC) type; };

// TODO: add specializations to std::wstring & const wchar_t*, typed strings...

// Helper that returns search param as such when there's no specialization of KeySearchTypeToInternalSearchType.
// TODO: check types, this will e.g. convert non-const ref to const ref.
template <class Key_T, class Search_T> const Search_T& makeInternalSearchType(const Search_T& t, std::true_type) { return t; }

// Helper that returns specialization as specified by KeySearchTypeToInternalSearchType.
template <class Key_T, class Search_T>
typename KeySearchTypeToInternalSearchType<Key_T, Search_T>::type makeInternalSearchType(const Search_T& t, std::false_type)
{
    return typename KeySearchTypeToInternalSearchType<Key_T, Search_T>::type(t);
}

template <class Key_T, class Search_T>
auto makeInternalSearchType(const Search_T& t) -> typename std::conditional<std::is_same<Search_T, typename KeySearchTypeToInternalSearchType<Key_T, Search_T>::type>::value, const Search_T&, typename KeySearchTypeToInternalSearchType<Key_T, Search_T>::type>::type
{
    typedef typename KeySearchTypeToInternalSearchType<Key_T, Search_T>::type InternalSearchType;
    const auto hasSpecializedSearchType = std::is_same<Search_T, InternalSearchType>();
    return makeInternalSearchType<Key_T, Search_T>(t, hasSpecializedSearchType);
}

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
    static auto findImpl(This_T& rThis, const T& keyRaw) -> decltype(rThis.makeIterator(0)) // iterator // std::conditional<std::is_const<This_T>::value, const_iterator, iterator>::type //, decltype(rThis.makeIterator(0))
    {
        const auto& key = makeInternalSearchType<typename This_T::key_type>(keyRaw);
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
