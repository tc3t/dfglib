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
#include "../build/languageFeatureInfo.hpp"
#include "../dfgAssert.hpp"
#include "detail/keyContainerUtils.hpp"
#include "TrivialPair.hpp"
#include "Vector.hpp"
#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include "../rangeIterator.hpp"
#include "../iter/CustomAccessIterator.hpp"
#include "../numericTypeTools.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    namespace DFG_DETAIL_NS
    {
        template <class Key_T, class Value_T, class SoA_T>
        class IteratorMapVectorSoa
        {
        public:
            typedef std::random_access_iterator_tag iterator_category;
            typedef ptrdiff_t difference_type;
            typedef void value_type;
            typedef void pointer;
            typedef void reference;

            template <class ValueRef_T>
            class PairHelperT : public std::pair<const Key_T&, ValueRef_T>
            {
            public:
                typedef std::pair<const Key_T&, ValueRef_T> BaseClass;

                PairHelperT(const Key_T& k, ValueRef_T& v) : BaseClass(k, v) {}
                BaseClass* operator->() { return this; }
            };

            using PairHelper = PairHelperT<Value_T&>;
            using PairHelperConst = PairHelperT<const Value_T&>;

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
                DFG_ASSERT_UB(this->m_pMap != nullptr);
                return PairHelper(m_pMap->m_keyStorage[m_i], m_pMap->m_valueStorage[m_i]);
            }

            PairHelperConst operator->() const
            {
                DFG_ASSERT_UB(this->m_pMap != nullptr);
                return PairHelperConst(m_pMap->m_keyStorage[m_i], m_pMap->m_valueStorage[m_i]);
            }

            const PairHelper operator*()
            {
                return operator->();
            }

            const PairHelperConst operator*() const
            {
                return operator->();
            }

            bool operator<(const IteratorMapVectorSoa& other) const
            {
                DFG_ASSERT_CORRECTNESS(this->m_pMap == other.m_pMap);
                return this->m_i < other.m_i;
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
        }; // Class IteratorMapVectorSoa

        template <class T> struct MapVectorTraits;

        template <class Impl_T>
        class MapVectorCrtp : public DFG_DETAIL_NS::KeyContainerBase<MapVectorTraits<Impl_T>>
        {
        public:
            typedef DFG_DETAIL_NS::KeyContainerBase<MapVectorTraits<Impl_T>> BaseClass;
            typedef typename BaseClass::iterator                             iterator;
            typedef typename BaseClass::const_iterator                       const_iterator;
            typedef typename BaseClass::key_iterator                         key_iterator;
            typedef typename MapVectorTraits<Impl_T>::const_key_iterator     const_key_iterator;
            typedef typename MapVectorTraits<Impl_T>::mapped_type_iterator   mapped_type_iterator;
            typedef typename MapVectorTraits<Impl_T>::mapped_type_const_iterator mapped_type_const_iterator;
            
            typedef typename BaseClass::key_type                             key_type;
            typedef typename MapVectorTraits<Impl_T>::mapped_type            mapped_type;
            typedef size_t                                                   size_type;

            MapVectorCrtp()
            {}

            bool                empty() const       { return static_cast<const Impl_T&>(*this).empty(); }
            size_t              size() const        { return static_cast<const Impl_T&>(*this).size(); }
            void                clear() const       { return static_cast<const Impl_T&>(*this).clear(); }

            iterator            makeIterator(const size_t i)          { return static_cast<Impl_T&>(*this).makeIterator(i); }
            const_iterator      makeIterator(const size_t i) const    { return static_cast<const Impl_T&>(*this).makeIterator(i); }

            key_iterator        makeKeyIterator(const size_t i)       { return static_cast<Impl_T&>(*this).makeKeyIterator(i); }
            const_key_iterator  makeKeyIterator(const size_t i) const { return static_cast<const Impl_T&>(*this).makeKeyIterator(i); }

            mapped_type_iterator       makeValueIterator(const size_t i)       { return static_cast<Impl_T&>(*this).makeValueIterator(i); }
            mapped_type_const_iterator makeValueIterator(const size_t i) const { return static_cast<const Impl_T&>(*this).makeValueIterator(i); }

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

            mapped_type_iterator        beginValue()       { return makeValueIterator(0); }
            mapped_type_const_iterator  beginValue() const { return makeValueIterator(0); }
            mapped_type_iterator        endValue()         { return makeValueIterator(size()); }
            mapped_type_const_iterator  endValue() const   { return makeValueIterator(size()); }

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

            // Note: one must be careful when editing keys (e.g. maintaining order when isSorted() is enabled). To make accidental edits less likely,
            //       keyRange() is always const. keyRange_modifiable() provides editable keys if needed.
            RangeIterator_T<key_iterator>       keyRange_modifiable() { return makeRange(beginKey(), endKey()); }
            RangeIterator_T<const_key_iterator> keyRange() const      { return makeRange(beginKey(), endKey()); }

            RangeIterator_T<mapped_type_iterator>       valueRange()             { return makeRange(beginValue(), endValue()); }
            RangeIterator_T<mapped_type_const_iterator> valueRange() const       { return valueRange_const(); }
            RangeIterator_T<mapped_type_const_iterator> valueRange_const() const { return makeRange(beginValue(), endValue()); } // For convenience.

            // Precondition: iterator must be dereferenceable.
            // TODO: test
            mapped_type&        keyIteratorToValue(const key_iterator iter)             { return makeIteratorFromKeyIterator(iter)->second; }
            const mapped_type&  keyIteratorToValue(const const_key_iterator iter) const { return makeIteratorFromKeyIterator(iter)->second; }

            iterator    erase(iterator iterDeleteFrom)  { return (iterDeleteFrom != end()) ? erase(iterDeleteFrom, iterDeleteFrom + 1) : end(); }
            template <class T>
            size_type   erase(const T& key)             { const auto nInitialSize = this->size(); erase(find(key)); return (nInitialSize - size()); } // Returns the number of elements removed.
            iterator    erase(iterator iterRangeFirst, const iterator iterRangeEnd)
            {
                if (this->m_bSorted)
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

            template <class T> iterator         find(const T& key)          { return this->findImpl(*this, key); }
            template <class T> const_iterator   find(const T& key) const    { return this->findImpl(*this, key); }

            template <class T>
            mapped_type& operator[](T&& key)
            {
                auto iterKey = findInsertPos(*this, key);
                if (iterKey != endKey() && isKeyMatch(keyIterValueToKeyValue(*iterKey), key))
                    return makeIterator(iterKey - beginKey())->second;
                else
                {
                    auto iter = insertNonExistingTo(this->keyParamToInsertable(std::forward<T>(key)), mapped_type(), iterKey);
                    return iter->second;
                }
            }

            iterator        makeIteratorFromKeyIterator(const key_iterator& iterKey)                { return makeIterator(iterKey - beginKey()); }
            const_iterator  makeIteratorFromKeyIterator(const const_key_iterator& iterKey) const    { return makeIterator(iterKey - beginKey()); }

            template <class T0, class T1>
            static bool isKeyMatch(const T0& a, const T1& b)
            {
                return BaseClass::isKeyMatch(a, b);
            }

            template <class This_T, class T>
            static auto findInsertPos(This_T& rThis, const T& key) -> decltype(rThis.beginKey())
            { 
                const auto iterKeyBegin = rThis.beginKey();
                const auto iterKeyEnd = rThis.endKey();
                typedef typename BaseClass::template KeyIteratorValueToComparable<This_T> KeyIteratorValueToComparableTd;
                auto iter = DFG_MODULE_NS(alg)::findInsertPosLinearOrBinary(rThis.isSorted(), iterKeyBegin, iterKeyEnd, key, KeyIteratorValueToComparableTd(rThis));
                return iter;
            }

            template <class T> bool hasKey(const T& key) const              { return find(key) != end(); }

            iterator insertNonExisting(const key_type& key, mapped_type&& value) { key_type k = key; return insertNonExisting(std::move(k), std::move(value)); }
            iterator insertNonExisting(key_type&& key, mapped_type&& value)      { return static_cast<Impl_T&>(*this).insertNonExistingToImpl(std::move(key), std::move(value), findInsertPos(*this, key)); }

            iterator insertNonExistingTo(key_type&& key, mapped_type&& value, const key_iterator& insertPos) { return static_cast<Impl_T&>(*this).insertNonExistingToImpl(std::move(key), std::move(value), insertPos); }

            iterator insertNonExistingTo(const key_type& key, const mapped_type& value, const key_iterator& insertPos)
            {
                auto newKey = key;
                auto newMapped = value;
                return insertNonExistingTo(std::move(newKey), std::move(newMapped), insertPos);
            }

            iterator insertNonExistingTo(const key_type& key, mapped_type&& value, const key_iterator& insertPos)
            {
                auto newKey = key;
                return insertNonExistingTo(std::move(newKey), std::move(value), insertPos);
            }

            std::pair<iterator, bool> insert(std::pair<key_type, mapped_type>&& newVal)
            {
                return insert(std::move(newVal.first), std::move(newVal.second));
            }

            key_type&& privToInsertableKey(key_type&& k)            { return std::move(k); }
            key_type privToInsertableKey(const key_type& k)         { return k; }
            mapped_type&& privToInsertableValue(mapped_type&& v)    { return std::move(v); }
            mapped_type privToInsertableValue(const mapped_type& v) { return v; }

            template <class Key_T, class Val_T>
            std::pair<iterator, bool> insertImpl(Key_T&& key, Val_T&& val)
            {
                auto iterKey = findInsertPos(*this, key);
                if (iterKey != endKey() && isKeyMatch(keyIterValueToKeyValue(*iterKey), key))
                    return std::pair<iterator, bool>(makeIteratorFromKeyIterator(iterKey), false); // Already present
                else
                {
                    auto iter = insertNonExistingTo(privToInsertableKey(std::forward<Key_T>(key)), privToInsertableValue(std::forward<Val_T>(val)), iterKey);
                    return std::pair<iterator, bool>(iter, true);
                }
            }

            std::pair<iterator, bool> insert(key_type&& key, mapped_type&& val) // TODO: key-type to be a template parameter.
            {
                return insertImpl(std::move(key), std::move(val));
            }

            std::pair<iterator, bool> insert(const key_type& key, mapped_type&& val)
            {
                return insertImpl(key, std::move(val));
            }

            std::pair<iterator, bool> insert(key_type&& key, const mapped_type& val)
            {
                return insertImpl(std::move(key), val);
            }

            std::pair<iterator, bool> insert(const key_type& key, const mapped_type& val)
            {
                return insertImpl(key, val);
            }

            void reserve(const size_t nReserve) { static_cast<Impl_T&>(*this).reserve(nReserve); }

            size_t capacity() const             { return static_cast<const Impl_T&>(*this).capacity(); }

            void sort()                         { static_cast<Impl_T&>(*this).sort(); }

            void setSorting(const bool bSort, const bool bAssumeSorted = false)
            {
                if (bSort == this->m_bSorted)
                    return;
                if (bSort && !bAssumeSorted)
                    sort();
                this->m_bSorted = bSort;
            }

            // Creates map from iterable so that keys are 0-based indexes (requires key_type to be an integer type).
            // If iterable.size() > maxValueOf<key_type>(), returns map truncated to size maxValueOf<key_type>().
            // If bAllowMoving is true and iterable has modifiable elements, moves elements instead of copying.
            // Dev note: moving can't be determined solely on rvalueness of iterable: it works when it is an owning container,
            //           but not when it is an rvalue range wrapper.
            //           Example
            //              std::vector<std::string> strings{"abc"};
            //              auto m = MapVectorSoa<int, std::string>::makeIndexMapped(makeRange(strings)); // iterable is rvalue, but the underlying storage is not.
            //           Conceptually would need isOwningIterable() trait to detect this and even in that case still need control for non-owning iterables whether
            //           to copy or move.
            template <class Iterable_T>
            static Impl_T makeIndexMapped(Iterable_T&& iterable, const bool bAllowMoving)
            {
                Impl_T rv;
                key_type nIndex = 0;
                const auto nMaxIndex = NumericTraits<key_type>::maxValue;
                for (auto&& item : iterable)
                {
                    if (bAllowMoving)
                        rv.insertNonExistingTo(nIndex, std::move(item), rv.endKey());
                    else
                        rv.insertNonExistingTo(nIndex, item, rv.endKey());
                    if (nIndex == nMaxIndex) // Overflow guard, truncates returned map to size nMaxIndex
                        return rv;
                    ++nIndex;
                }
                return rv;
            }

            // Returns copy of value mapped to 'key' or if it doesn't exist, returns 'defaultValue'.
            template <class T>
            mapped_type valueCopyOr(const T& key, mapped_type defaultValue = mapped_type()) const
            {
                auto iter = find(key);
                return (iter != end()) ? iter->second : defaultValue;
            }

        }; // class MapVectorCrtp

        //template <class T> struct DefaultMapVectorContainerType { typedef std::vector<T> type; };
        template <class T> struct DefaultMapVectorContainerType { typedef Vector<T> type; };

    } // namespace DFG_DETAIL_NS

    

template <class Key_T, class Value_T, class KeyStorage_T = typename DFG_DETAIL_NS::DefaultMapVectorContainerType<Key_T>::type, class ValueStorage_T = typename DFG_DETAIL_NS::DefaultMapVectorContainerType<Value_T>::type>
class MapVectorSoA : public DFG_DETAIL_NS::MapVectorCrtp<MapVectorSoA<Key_T, Value_T>>
{
public:
    typedef DFG_DETAIL_NS::MapVectorCrtp<MapVectorSoA<Key_T, Value_T>>  BaseClass;
    typedef typename BaseClass::iterator                iterator;
    typedef typename BaseClass::const_iterator          const_iterator;
    typedef typename BaseClass::key_iterator            key_iterator;
    typedef typename BaseClass::const_key_iterator      const_key_iterator;
    typedef typename BaseClass::mapped_type_iterator    mapped_type_iterator;
    typedef typename BaseClass::mapped_type_const_iterator mapped_type_const_iterator;
    typedef typename BaseClass::key_type                key_type;
    typedef typename BaseClass::mapped_type             mapped_type;
    typedef typename BaseClass::size_type               size_type;

#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
    MapVectorSoA()
    {}

    MapVectorSoA(MapVectorSoA && other)
    {
        operator=(std::move(other));
    }

    MapVectorSoA(const MapVectorSoA& other)
    {
        operator=(other);
    }

    MapVectorSoA& operator=(const MapVectorSoA& other)
    {
        this->m_bSorted = other.m_bSorted;
        m_keyStorage = other.m_keyStorage;
        m_valueStorage = other.m_valueStorage;
        return *this;
    }

    MapVectorSoA& operator=(MapVectorSoA&& other)
    {
        this->m_bSorted = other.m_bSorted;
        m_keyStorage = std::move(other.m_keyStorage);
        m_valueStorage = std::move(other.m_valueStorage);
        return *this;
    }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

    // See MapVectorCrtp::makeIndexMapped() for documentation
    template <class Iterable_T>
    static MapVectorSoA makeIndexMapped(Iterable_T&& iterable, const bool bAllowMoving) { return BaseClass::makeIndexMapped(std::forward<Iterable_T>(iterable), bAllowMoving); }

    iterator            makeIterator(const size_t i)                        { return iterator(*this, i); }
    const_iterator      makeIterator(const size_t i) const                  { return const_iterator(*this, i); }

    key_iterator        makeKeyIterator(const size_t i)                     { return m_keyStorage.begin() + i; }
    const_key_iterator  makeKeyIterator(const size_t i) const               { return m_keyStorage.begin() + i; }

    mapped_type_iterator       makeValueIterator(const size_t i)            { return m_valueStorage.begin() + i; }
    mapped_type_const_iterator makeValueIterator(const size_t i) const      { return m_valueStorage.begin() + i; }

    Key_T&              keyIterValueToKeyValue(Key_T& keyVal)               { return keyVal; }
    const Key_T&        keyIterValueToKeyValue(const Key_T& keyVal) const   { return keyVal; }

    bool    empty() const   { return m_keyStorage.empty(); }
    size_t  size() const    { return m_keyStorage.size(); }
    void    clear()         { m_keyStorage.clear(); m_valueStorage.clear(); }

    iterator insertNonExistingToImpl(key_type&& key, mapped_type&& value, const key_iterator& insertPos)
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

    template <class KeyIterable_T, class ValueIterable_T>
    void pushBackToUnsorted(const KeyIterable_T& keyRange, const ValueIterable_T& valueRange)
    {
        if (this->isSorted() || keyRange.size() != valueRange.size() || keyRange.empty())
            return;
        const auto nSizeIncrement = keyRange.size();
        if (nSizeIncrement > m_keyStorage.max_size() || m_keyStorage.max_size() - nSizeIncrement < m_keyStorage.size())
            return;
        DFG_ASSERT_UB(this->size() == m_keyStorage.size() && this->size() == m_keyStorage.size());

        m_keyStorage.insert(m_keyStorage.end(), keyRange.begin(), keyRange.end());
        m_valueStorage.insert(m_valueStorage.end(), valueRange.begin(), valueRange.end());
    }

    template <class KeyIterable_T, class KeyTransform_T, class ValueIterable_T, class ValueTransform_T>
    void pushBackToUnsorted(const KeyIterable_T& keyRange, KeyTransform_T funcKey, const ValueIterable_T& valueRange, ValueTransform_T&& funcValue)
    {
        if (this->isSorted() || keyRange.size() != valueRange.size() || keyRange.empty())
            return;
        const auto nSizeIncrement = keyRange.size();
        if (nSizeIncrement > m_keyStorage.max_size() || m_keyStorage.max_size() - nSizeIncrement < m_keyStorage.size())
            return;
        DFG_ASSERT_UB(size() == m_keyStorage.size() && size() == m_keyStorage.size());
        const auto nOldSize = size();
        m_keyStorage.resize(m_keyStorage.size() + nSizeIncrement);
        m_valueStorage.resize(m_valueStorage.size() + nSizeIncrement);

        std::transform(keyRange.begin(), keyRange.end(), m_keyStorage.begin() + nOldSize, funcKey);
        std::transform(valueRange.begin(), valueRange.end(), m_valueStorage.begin() + nOldSize, funcValue);
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


//template <class Key_T, class Value_T, class Storage_T = typename DFG_DETAIL_NS::DefaultMapVectorContainerType<std::pair<Key_T, Value_T>>::type>
template <class Key_T, class Value_T, class Storage_T = typename DFG_DETAIL_NS::DefaultMapVectorContainerType<TrivialPair<Key_T, Value_T>>::type>
class MapVectorAoS : public DFG_DETAIL_NS::MapVectorCrtp<MapVectorAoS<Key_T, Value_T>>
{
public:
    typedef DFG_DETAIL_NS::MapVectorCrtp<MapVectorAoS<Key_T, Value_T>>      BaseClass;
    typedef DFG_DETAIL_NS::MapVectorTraits<MapVectorAoS<Key_T, Value_T>>    TraitsType;
    
    typedef Storage_T                                           StorageType;
    typedef typename StorageType::value_type                    value_type;
    typedef typename BaseClass::iterator                        iterator;
    typedef typename BaseClass::const_iterator                  const_iterator;
    typedef typename BaseClass::key_iterator                    key_iterator;
    typedef typename BaseClass::const_key_iterator              const_key_iterator;
    typedef typename BaseClass::mapped_type_iterator            mapped_type_iterator;
    typedef typename BaseClass::mapped_type_const_iterator      mapped_type_const_iterator;
    typedef typename BaseClass::key_type                        key_type;
    typedef typename BaseClass::mapped_type                     mapped_type;
    typedef typename BaseClass::size_type                       size_type;
    
#if !DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT
    MapVectorAoS()
    {}

    MapVectorAoS(MapVectorAoS&& other)
    {
        operator=(std::move(other));
    }

    MapVectorAoS(const MapVectorAoS& other)
    {
        operator=(other);
    }

    MapVectorAoS& operator=(const MapVectorAoS& other)
    {
        this->m_bSorted = other.m_bSorted;
        m_storage = other.m_storage;
        return *this;
    }

    MapVectorAoS& operator=(MapVectorAoS&& other)
    {
        this->m_bSorted = other.m_bSorted;
        m_storage = std::move(other.m_storage);
        return *this;
    }
#endif // DFG_LANGFEAT_AUTOMATIC_MOVE_CTOR_AND_ASSIGNMENT

    // See MapVectorCrtp::makeIndexMapped() for documentation
    template <class Iterable_T>
    static MapVectorAoS makeIndexMapped(Iterable_T&& iterable, const bool bAllowMoving) { return BaseClass::makeIndexMapped(std::forward<Iterable_T>(iterable), bAllowMoving); }

    iterator           makeIterator(const size_t i)            { return m_storage.begin() + i; }
    const_iterator     makeIterator(const size_t i) const      { return m_storage.begin() + i; }

    key_iterator       makeKeyIterator(const size_t i)         { return key_iterator(makeIterator(i)); }
    const_key_iterator makeKeyIterator(const size_t i) const   { return const_key_iterator(makeIterator(i)); }

    mapped_type_iterator       makeValueIterator(const size_t i)       { return mapped_type_iterator(makeIterator(i)); }
    mapped_type_const_iterator makeValueIterator(const size_t i) const { return mapped_type_const_iterator(makeIterator(i)); }

    key_type&       keyIterValueToKeyValue(key_type& val)               { return val; }
    const key_type& keyIterValueToKeyValue(const key_type& val) const   { return val; }

    bool    empty() const   { return m_storage.empty(); }
    size_t  size() const    { return m_storage.size(); }
    void    clear()         { m_storage.clear(); }

    iterator insertNonExistingToImpl(key_type&& key, mapped_type&& value, const key_iterator& iter)
    {
        return m_storage.insert(makeIterator(iter - this->beginKey()), value_type(std::move(key), std::move(value)));
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
    struct MapVectorTraits<MapVectorSoA<Key_T, Value_T, KeyStorage_T, ValueStorage_T>>
    {
        typedef MapVectorSoA<Key_T, Value_T>            ImplT;
        typedef Key_T                                                   key_type;
        typedef Value_T                                                 mapped_type;
        typedef IteratorMapVectorSoa<Key_T, Value_T, ImplT>             iterator;
        typedef IteratorMapVectorSoa<Key_T, const Value_T, const ImplT> const_iterator;
        typedef typename KeyStorage_T::iterator                         key_iterator;
        typedef typename KeyStorage_T::const_iterator                   const_key_iterator;
        typedef typename ValueStorage_T::iterator                       mapped_type_iterator;
        typedef typename ValueStorage_T::const_iterator                 mapped_type_const_iterator;
    };

    template <class Key_T, class Value_T, class Storage_T>
    struct MapVectorTraits<MapVectorAoS<Key_T, Value_T, Storage_T>>
    {
        typedef MapVectorAoS<Key_T, Value_T>            ImplT;
        typedef Key_T                                                   key_type;
        typedef Value_T                                                 mapped_type;
        typedef typename Storage_T::iterator                            iterator;
        typedef typename Storage_T::const_iterator                      const_iterator;

        using key_iterator       = decltype(::DFG_MODULE_NS(iter)::makeTupleElementAccessIterator<0>(iterator()));
        using const_key_iterator = decltype(::DFG_MODULE_NS(iter)::makeTupleElementAccessIterator<0>(const_iterator()));
        using mapped_type_iterator       = decltype(::DFG_MODULE_NS(iter)::makeTupleElementAccessIterator<1>(iterator()));
        using mapped_type_const_iterator = decltype(::DFG_MODULE_NS(iter)::makeTupleElementAccessIterator<1>(const_iterator()));
    };
} // namespace DFG_DETAIL_NS

}} // Module namespace
