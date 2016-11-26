#pragma once

/*
MapVector.hpp

Implements contiguous storage maps with the following properties:
    -Guarantees contiguous storage, both SoA and AoS implementations.
    -Sorting is controlled by a per-object runtime flag and thus searching is either linear or logarithmic.
    -Invalidation of iterators is dependent on runtime properties, especially capacity and sorting.
        -Insert to map with size() == capacity() will invalidate all iterators.
        -Insert to unsorted map with size() < capacity() will invalidate only end() iterator.
        -Insert to sorted map with size() < capacity() invalidates iterators only for those items whose key is after inserted key.
        -Single deletion from unsorted map invalidates only iterators of deleted item, last item and end(). TODO: revise this.
        -Deletion from sorted map invalidates only those items whose key is after deleted key. // TODO: verify behaviour of vector erase.
    -Provides reserve() for preallocation.
    -Allows keys to be modified (programmer responsibility to use it correctly).
    -Keys can be searched without contructing key_type (for example std::string keys can be searched with const char* without constructing std::string)
    -Currently requires key_type to have operator==.

There are two concrete implementations (that use a common CRTP-base):
    -MapVectorAoS<key_type, mapped_type> [use by default]
        -Keys and values are stored in pair-structure.
        -Can be made highly drop-in-replacable with std::map; currently (2016-11) much is missing.
    -MapVectorSoA<key_type, mapped_type> [more experimental]
        -Keys and values are stored in separate contiguous arrays.
        -Less compatible with std::map, i.e. less likely to work as drop-in-replacement (even when implemented fully). This is mainly because the storage does not allow simple iterator over pairs.

Related reading and implementations:
    -"A Standard flat_map" http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0429r0.pdf
    -boost::container::flat_map
    -http://stackoverflow.com/a/25027750 (Answer in thread "boost::flat_map and its performance compared to map and unordered_map")
*/

#include "../dfgDefs.hpp"
#include "../alg/eraseByTailSwap.hpp"
#include "../alg/find.hpp"
#include "../alg/sortMultiple.hpp"
#include "TrivialPair.hpp"
#include "Vector.hpp"
#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    namespace DFG_DETAIL_NS
    {
        template <class Key_T, class Value_T, class SoA_T>
        class IteratorMapVectorSoa
        {
        public:
            typedef std::random_access_iterator_tag iterator_category;

            typedef Value_T& ValueRefType;
            class PairHelper : public std::pair<const Key_T&, ValueRefType>
            {
            public:
                typedef std::pair<const Key_T&, ValueRefType> BaseClass;

                PairHelper(const Key_T& k, Value_T& v) : BaseClass(k, v) {}
                BaseClass* operator->() { return this; }
            };


            IteratorMapVectorSoa(SoA_T& rMap, size_t i) :
                m_pMap(&rMap),
                m_i(i)
            {
            }

            IteratorMapVectorSoa& operator++()
            {
                ++m_i;
                return *this;
            }

            IteratorMapVectorSoa& operator--()
            {
                --m_i;
                return *this;
            }

            PairHelper operator->()
            {
                return PairHelper(m_pMap->m_keyStorage[m_i], m_pMap->m_valueStorage[m_i]);
            }

            IteratorMapVectorSoa operator+(const ptrdiff_t diff) const
            {
                return IteratorMapVectorSoa(*m_pMap, m_i + diff);
            }

            IteratorMapVectorSoa operator-(const ptrdiff_t diff) const
            {
                return IteratorMapVectorSoa(*m_pMap, m_i - diff);
            }

            ptrdiff_t operator-(const IteratorMapVectorSoa& other) const
            {
                return m_i - other.m_i;
            }

            bool operator==(const IteratorMapVectorSoa& other) const
            {
                return m_i == other.m_i;
            }

            bool operator!=(const IteratorMapVectorSoa& other) const
            {
                return !(*this == other);
            }

            SoA_T* m_pMap;
            size_t m_i;
        };

        template <class T> struct MapVectorTraits;

        template <class Impl_T>
        class MapVectorCrtp
        {
        public:
            typedef typename MapVectorTraits<Impl_T>::iterator              iterator;
            typedef typename MapVectorTraits<Impl_T>::const_iterator        const_iterator;
            typedef typename MapVectorTraits<Impl_T>::key_iterator          key_iterator;
            typedef typename MapVectorTraits<Impl_T>::const_key_iterator    const_key_iterator;
            typedef typename MapVectorTraits<Impl_T>::key_type              key_type;
            typedef typename MapVectorTraits<Impl_T>::mapped_type           mapped_type;
            typedef size_t                                                  size_type;

            MapVectorCrtp() :
                m_bSorted(true)
            {}

            bool                empty() const       { return static_cast<const Impl_T&>(*this).empty(); }
            size_t              size() const        { return static_cast<const Impl_T&>(*this).size(); }
            void                clear() const       { return static_cast<const Impl_T&>(*this).clear(); }

            iterator            makeIterator(const size_t i)        { return static_cast<Impl_T&>(*this).makeIterator(i); }
            const_iterator      makeIterator(const size_t i) const  { return static_cast<const Impl_T&>(*this).makeIterator(i); }

            key_iterator        makeKeyIterator(const size_t i)       { return static_cast<Impl_T&>(*this).makeKeyIterator(i); }
            const_key_iterator  makeKeyIterator(const size_t i) const { return static_cast<const Impl_T&>(*this).makeKeyIterator(i); }

            iterator            begin()             { return makeIterator(0); }
            const_iterator      begin() const       { return makeIterator(0); }
            const_iterator      cbegin() const      { return makeIterator(0); }
            iterator            end()               { return makeIterator(size()); }
            const_iterator      end() const         { return makeIterator(size()); }
            const_iterator      cend() const        { return makeIterator(size()); }

            iterator            backIter()          { return end() - 1; }
            const_iterator      backIter() const    { return end() - 1; }

            key_iterator        beginKey()          { return makeKeyIterator(0); }
            const_key_iterator  beginKey() const    { return makeKeyIterator(0); }
            key_iterator        endKey()            { return makeKeyIterator(size()); }
            const_key_iterator  endKey() const      { return makeKeyIterator(size()); }

            key_type&           frontKey()          { return keyIterValueToKeyValue(*beginKey()); }
            const key_type&     frontKey() const    { return keyIterValueToKeyValue(*beginKey()); }
            mapped_type&        frontValue()        { return begin()->second; }
            const mapped_type&  frontValue() const  { return begin()->second; }

            key_type&           backKey()           { return keyIterValueToKeyValue(*backKeyIter()); }
            const key_type&     backKey() const     { return keyIterValueToKeyValue(*backKeyIter()); }
            mapped_type&        backValue()         { return backIter()->second; }
            const mapped_type&  backValue() const   { return backIter()->second; }
            key_iterator        backKeyIter()       { return endKey() - 1; }
            const_key_iterator  backKeyIter() const { return endKey() - 1; }

            iterator    erase(iterator iterDeleteFrom)  { return (iterDeleteFrom != end()) ? erase(iterDeleteFrom, iterDeleteFrom + 1) : end(); }
            template <class T>
            size_type   erase(const T& key)             { auto rv = erase(find(key)); return (rv != end()) ? 1 : 0; } // Returns the number of elements removed.
            iterator    erase(iterator iterRangeFirst, const iterator iterRangeEnd)
            {
                if (m_bSorted)
                    return static_cast<Impl_T&>(*this).eraseImpl(iterRangeFirst, iterRangeEnd);
                else // With unsorted, swap to-be-removed items to end and erase items from end -> O(1) for single element removal.
                {
                    return DFG_MODULE_NS(alg)::eraseByTailSwap(*this, iterRangeFirst, iterRangeEnd,
                                                                [&](const iterator& i0, const iterator& i1) { static_cast<Impl_T&>(*this).swapElem(i0, i1); },
                                                                [&](const iterator& i0, const iterator& i1) { static_cast<Impl_T&>(*this).eraseImpl(i0, i1); });
                }
            }

            key_type&       keyIterValueToKeyValue(typename key_iterator::value_type& val)              { return static_cast<Impl_T&>(*this).keyIterValueToKeyValue(val); }
            const key_type& keyIterValueToKeyValue(const typename key_iterator::value_type& val) const  { return static_cast<const Impl_T&>(*this).keyIterValueToKeyValue(val); }

            key_type&&                  keyParamToInsertable(key_type&& key)        { return std::move(key); }
            template <class T> key_type keyParamToInsertable(const T& keyParam)     { return key_type(keyParam); }

            template <class T>
            mapped_type& operator[](T&& key)
            {
                auto iterKey = findInsertPos(*this, key);
                if (iterKey != endKey() && isKeyMatch(keyIterValueToKeyValue(*iterKey), key))
                    return makeIterator(iterKey - beginKey())->second;
                else
                {
                    auto iter = insertNonExistingTo(keyParamToInsertable(std::forward<T>(key)), mapped_type(), iterKey);
                    return iter->second;
                }
            }

            iterator        makeIteratorFromKeyIterator(const key_iterator& iterKey)         { return makeIterator(iterKey - beginKey()); }
            const_iterator  makeIteratorFromKeyIterator(const key_iterator& iterKey) const   { return makeIterator(iterKey - beginKey()); }

            template <class T0, class T1>
            static bool isKeyMatch(const T0& a, const T1& b)
            {
                return DFG_MODULE_NS(alg)::isKeyMatch(a, b);
            }

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

            template <class This_T, class T>
            static auto findInsertPos(This_T& rThis, const T& key) -> decltype(rThis.beginKey())
            { 
                const auto iterKeyBegin = rThis.beginKey();
                const auto iterKeyEnd = rThis.endKey();
                auto iter = DFG_MODULE_NS(alg)::findInsertPosLinearOrBinary(rThis.isSorted(), iterKeyBegin, iterKeyEnd, key, KeyIteratorValueToComparable<This_T>(rThis));
                return iter;
            }

            template <class This_T, class T>
            static auto findImpl(This_T& rThis, const T& key) -> decltype(rThis.makeIterator(0))
            {
                if (rThis.isSorted())
                {
                    auto iter = findInsertPos(rThis, key);
                    return (iter != rThis.endKey() && isKeyMatch(rThis.keyIterValueToKeyValue(*iter), key)) ? rThis.makeIterator(iter - rThis.beginKey()) : rThis.end();
                }
                else 
                    return rThis.makeIterator(DFG_MODULE_NS(alg)::findLinear(rThis.beginKey(), rThis.endKey(), key, KeyIteratorValueToComparable<This_T>(rThis)) - rThis.beginKey());
            }

            template <class T> iterator         find(const T& key)          { return findImpl(*this, key); }
            template <class T> const_iterator   find(const T& key) const    { return findImpl(*this, key); }

            template <class T> bool hasKey(const T& key) const              { return find(key) != end(); }

            iterator insertNonExisting(const key_type& key, mapped_type&& value) { key_type k = key; return insertNonExisting(std::move(k), std::move(value)); }
            iterator insertNonExisting(key_type&& key, mapped_type&& value)      { return static_cast<Impl_T&>(*this).insertNonExistingTo(std::move(key), std::move(value), findInsertPos(*this, key)); }

            iterator insertNonExistingTo(key_type&& key, mapped_type&& value, const key_iterator& insertPos) { return static_cast<Impl_T&>(*this).insertNonExistingTo(std::move(key), std::move(value), insertPos); }

            std::pair<iterator, bool> insert(std::pair<key_type, mapped_type>&& newVal)
            {
                return insert(std::move(newVal.first), std::move(newVal.second));
            }

            std::pair<iterator, bool> insert(key_type&& key, mapped_type&& val) // TODO: key-type to be a template parameter.
            {
                auto iterKey = findInsertPos(*this, key);
                if (iterKey != endKey() && isKeyMatch(keyIterValueToKeyValue(*iterKey), key))
                    return std::pair<iterator, bool>(makeIteratorFromKeyIterator(iterKey), false); // Already present
                else
                {
                    auto iter = insertNonExistingTo(std::move(key), std::move(val), iterKey);
                    return std::pair<iterator, bool>(iter, true);
                }
            }

            void reserve(const size_t nReserve) { static_cast<Impl_T&>(*this).reserve(nReserve); }

            size_t capacity() const             { return static_cast<const Impl_T&>(*this).capacity(); }

            void sort()                         { static_cast<Impl_T&>(*this).sort(); }

            void setSorting(const bool bSort)
            {
                if (bSort == m_bSorted)
                    return;
                if (bSort)
                    sort();
                m_bSorted = bSort;
            }

            bool isSorted() const { return m_bSorted; }

            bool m_bSorted;
        }; // class MapVectorCrtp

        template <class T> struct DefaultContainerType { typedef std::vector<T> type; };
        //template <class T> struct DefaultContainerType { typedef DFG_CLASS_NAME(Vector)<T> type; };

    } // namespace DFG_DETAIL_NS

    

template <class Key_T, class Value_T, class KeyStorage_T = typename DFG_DETAIL_NS::DefaultContainerType<Key_T>::type, class ValueStorage_T = typename DFG_DETAIL_NS::DefaultContainerType<Value_T>::type>
class DFG_CLASS_NAME(MapVectorSoA) : public DFG_DETAIL_NS::MapVectorCrtp<DFG_CLASS_NAME(MapVectorSoA)<Key_T, Value_T>>
{
public:
    typedef DFG_DETAIL_NS::MapVectorCrtp<DFG_CLASS_NAME(MapVectorSoA)<Key_T, Value_T>>  BaseClass;
    typedef typename BaseClass::iterator                iterator;
    typedef typename BaseClass::const_iterator          const_iterator;
    typedef typename BaseClass::key_iterator            key_iterator;
    typedef typename BaseClass::const_key_iterator      const_key_iterator;
    typedef typename BaseClass::key_type                key_type;
    typedef typename BaseClass::mapped_type             mapped_type;
    typedef typename BaseClass::size_type               size_type;

    iterator            makeIterator(const size_t i)                        { return iterator(*this, i); }
    const_iterator      makeIterator(const size_t i) const                  { return const_iterator(*this, i); }

    key_iterator        makeKeyIterator(const size_t i)                     { return m_keyStorage.begin() + i; }
    const_key_iterator  makeKeyIterator(const size_t i) const               { return m_keyStorage.begin() + i; }

    Key_T&              keyIterValueToKeyValue(Key_T& keyVal)               { return keyVal; }
    const Key_T&        keyIterValueToKeyValue(const Key_T& keyVal) const   { return keyVal; }

    bool    empty() const   { return m_keyStorage.empty(); }
    size_t  size() const    { return m_keyStorage.size(); }
    void    clear()         { m_keyStorage.clear(); m_valueStorage.clear(); }

    iterator insertNonExistingTo(key_type&& key, mapped_type&& value, const key_iterator& insertPos)
    {
        const auto nIndex = insertPos - m_keyStorage.begin();
        m_keyStorage.insert(insertPos, std::move(key));
        m_valueStorage.insert(m_valueStorage.begin() + nIndex, std::move(value));
        return makeIterator(nIndex);
    }

    void reserve(const size_t nReserve)
    {
        m_keyStorage.reserve(nReserve);
        m_valueStorage.reserve(nReserve);
    }

    size_t capacity() const
    {
        return m_keyStorage.capacity();
    }

    void sort()
    {
        ::DFG_MODULE_NS(alg)::sortMultiple(m_keyStorage, m_valueStorage);
    }

    iterator eraseImpl(iterator iterRangeFirst, iterator iterRangeEnd)
    { 
        const auto nFirst = iterRangeFirst - this->begin();
        const auto nEnd = iterRangeEnd - this->begin();
        m_keyStorage.erase(m_keyStorage.begin() + nFirst, m_keyStorage.begin() + nEnd);
        m_valueStorage.erase(m_valueStorage.begin() + nFirst, m_valueStorage.begin() + nEnd);
        return makeIterator(nFirst);
    }

    void swapElem(const iterator& a, const iterator& b)
    {
        const auto nIndexA = a - this->begin();
        const auto nIndexB = b - this->begin();
        using std::swap;
        swap(m_keyStorage[nIndexA], m_keyStorage[nIndexB]);
        swap(m_valueStorage[nIndexA], m_valueStorage[nIndexB]);
    }

    KeyStorage_T    m_keyStorage;
    ValueStorage_T  m_valueStorage;
}; // class MapVectorSoA


//template <class Key_T, class Value_T, class Storage_T = typename DFG_DETAIL_NS::DefaultContainerType<DFG_CLASS_NAME(TrivialPair)<Key_T, Value_T>>::type>
template <class Key_T, class Value_T, class Storage_T = typename DFG_DETAIL_NS::DefaultContainerType<std::pair<Key_T, Value_T>>::type>
class DFG_CLASS_NAME(MapVectorAoS) : public DFG_DETAIL_NS::MapVectorCrtp<DFG_CLASS_NAME(MapVectorAoS)<Key_T, Value_T>>
{
public:
    typedef DFG_DETAIL_NS::MapVectorCrtp<DFG_CLASS_NAME(MapVectorAoS)<Key_T, Value_T>>      BaseClass;
    typedef DFG_DETAIL_NS::MapVectorTraits<DFG_CLASS_NAME(MapVectorAoS)<Key_T, Value_T>>    TraitsType;
    
    typedef Storage_T                                           StorageType;
    typedef typename StorageType::value_type                    value_type;
    typedef typename BaseClass::iterator                        iterator;
    typedef typename BaseClass::const_iterator                  const_iterator;
    typedef typename BaseClass::key_iterator                    key_iterator;
    typedef typename BaseClass::const_key_iterator              const_key_iterator;
    typedef typename BaseClass::key_type                        key_type;
    typedef typename BaseClass::mapped_type                     mapped_type;
    typedef typename BaseClass::size_type                       size_type;
    
    

    key_iterator        makeIterator(const size_t i)            { return m_storage.begin() + i; }
    const_key_iterator  makeIterator(const size_t i) const      { return m_storage.begin() + i; }

    iterator            makeKeyIterator(const size_t i)         { return makeIterator(i); }
    const_iterator      makeKeyIterator(const size_t i) const   { return makeIterator(i); }

    key_type&       keyIterValueToKeyValue(value_type& val)               { return val.first; }
    const key_type& keyIterValueToKeyValue(const value_type& val) const   { return val.first; }

    bool    empty() const   { return m_storage.empty(); }
    size_t  size() const    { return m_storage.size(); }
    void    clear()         { m_storage.clear(); }

    iterator insertNonExistingTo(key_type&& key, mapped_type&& value, const key_iterator& iter)
    {
        return m_storage.insert(iter, value_type(std::move(key), std::move(value)));
    }

    void reserve(const size_t nReserve)
    {
        m_storage.reserve(nReserve);
    }

    size_t capacity() const
    {
        return m_storage.capacity();
    }

    void sort()
    {
        std::sort(m_storage.begin(), m_storage.end(), [](const value_type& left, const value_type& right)
        {
            return left.first < right.first;
        });
    }

    iterator eraseImpl(iterator iterRangeFirst, iterator iterRangeEnd)
    {
        return m_storage.erase(iterRangeFirst, iterRangeEnd);
    }

    void swapElem(const iterator& a, const iterator& b)
    {
        std::iter_swap(a, b);
    }

    StorageType m_storage;
}; // class MapVectorAoS


namespace DFG_DETAIL_NS
{
    template <class Key_T, class Value_T, class KeyStorage_T, class ValueStorage_T>
    struct MapVectorTraits<DFG_CLASS_NAME(MapVectorSoA)<Key_T, Value_T, KeyStorage_T, ValueStorage_T>>
    {
        typedef DFG_CLASS_NAME(MapVectorSoA)<Key_T, Value_T>            ImplT;
        typedef Key_T                                                   key_type;
        typedef Value_T                                                 mapped_type;
        typedef IteratorMapVectorSoa<Key_T, Value_T, ImplT>             iterator;
        typedef IteratorMapVectorSoa<Key_T, const Value_T, const ImplT> const_iterator;
        typedef typename KeyStorage_T::iterator                         key_iterator;
        typedef typename KeyStorage_T::const_iterator                   const_key_iterator;
    };

    template <class Key_T, class Value_T, class Storage_T>
    struct MapVectorTraits<DFG_CLASS_NAME(MapVectorAoS)<Key_T, Value_T, Storage_T>>
    {
        typedef DFG_CLASS_NAME(MapVectorAoS)<Key_T, Value_T>            ImplT;
        typedef Key_T                                                   key_type;
        typedef Value_T                                                 mapped_type;
        typedef typename Storage_T::iterator                            iterator;
        typedef typename Storage_T::const_iterator                      const_iterator;
        typedef iterator                                                key_iterator;
        typedef const_iterator                                          const_key_iterator;
    };
} // namespace DFG_DETAIL_NS

}} // Module namespace
