#pragma once

#include "../dfgDefs.hpp"
#include "../dfgAssert.hpp"
#include "../dfgBase.hpp"
#include "../ReadOnlySzParam.hpp"
#include "MapVector.hpp"

#include <type_traits>
#include <vector>


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

enum class StringStorageType
{
    sizeAndNullTerminated, // Strings are stored with both size and null terminator.
    sizeOnly,              // Strings are stored with size but are not null terminated.
    szSized                // Strings are stored null terminated without size, i.e. size is based on null terminator so with this option may not store strings that have embedded nulls. 
};

// Owning container for Key_T -> string view map.
// Strings are not stored as list of individual string objects but as char array(s) to which views point to.
// StringStorageType_T determines string storage type
// About iterators: to access string view from iterator, call second-item's operator()() with this class as argument.
// Note: operator[] works differently from std::map: inserts can't be done with syntax m[key] = value;
template <class Key_T, class String_T, StringStorageType StringStorageType_T = StringStorageType::sizeAndNullTerminated, class SizeType_T = std::size_t>
class MapToStringViews
{
public:
    using CharT                  = typename String_T::value_type;
    using StringView             = ::DFG_ROOT_NS::StringView<CharT, String_T>;
    using StringViewSz           = ::DFG_ROOT_NS::StringViewSz<CharT, typename StringView::StringT>;
    using AreViewsNullTerminated = typename std::conditional<StringStorageType_T != StringStorageType::sizeOnly, std::true_type, std::false_type>::type;
    using StringViewRv           = typename std::conditional<AreViewsNullTerminated::value, StringViewSz, StringView>::type;
    using size_type              = SizeType_T;
    using PtrT                   = typename StringView::PtrT;
    using SzPtrT                 = typename StringView::SzPtrT;
    using DataStorage            = std::vector<CharT>;

private:
    // For szSized-storage where only start pos is stored.
    class StringDetailSzSized
    {
    public:
        size_type m_nStartPos;
        StringDetailSzSized(const size_type nStartPos = 0, const size_type nEndPos = 0)
            : m_nStartPos(nStartPos)
        {
            DFG_UNUSED(nEndPos);
        }
        StringViewRv toView(const DataStorage& data) const
        {
            return StringViewRv(SzPtrT(data.data() + m_nStartPos));
        }

        StringViewRv operator()(const MapToStringViews& cont) const
        {
            return toView(cont.m_data);
        }
    };
    class StringDetailSizedView
    {
    public:
        size_type m_nStartPos;
        size_type m_nEndPos;
        StringDetailSizedView(const size_type nStartPos = 0, const size_type nEndPos = 0)
            : m_nStartPos(nStartPos)
            , m_nEndPos(nEndPos)
        {
        }
        StringViewRv toView(const DataStorage& data) const
        {
            // Checking that interpreting (data.data() + m_nStartPos) as SzPtrT is valid if using SzView.
            DFG_ASSERT_UB((!std::is_same<StringViewRv, StringViewSz>::value || data[m_nEndPos] == '\0'));
            return StringViewRv(SzPtrT(data.data() + m_nStartPos), m_nEndPos - m_nStartPos);
        }

        StringViewRv operator()(const MapToStringViews& cont) const
        {
            return toView(cont.m_data);
        }
    };

public:
    using StringDetail   = typename std::conditional<StringStorageType_T == StringStorageType::szSized, StringDetailSzSized, StringDetailSizedView>::type;
    using KeyStorage     = MapVectorAoS<Key_T, StringDetail>;
    using const_iterator = typename KeyStorage::const_iterator;

    MapToStringViews()
    {
        m_data.push_back('\0');
    }

    const_iterator begin() const
    {
        return m_keyToStringDetails.begin();
    }

    const_iterator end() const
    {
        return m_keyToStringDetails.end();
    }

    // Precondition: !empty()
    const Key_T&       frontKey()   const { DFG_ASSERT_UB(!this->empty()); return this->begin()->first;             } 
    const StringViewRv frontValue() const { DFG_ASSERT_UB(!this->empty()); return this->begin()->second(*this);     }
    const Key_T&       backKey()    const { DFG_ASSERT_UB(!this->empty()); return (this->end() - 1)->first;         }
    const StringViewRv backValue()  const { DFG_ASSERT_UB(!this->empty()); return (this->end() - 1)->second(*this); }

    // Inserts mapping key -> view. If key already exists, behaviour is unspecified. TODO: specify
    void insert(const Key_T& key, const StringView& sv)
    {
        DFG_ASSERT_CORRECTNESS(!m_data.empty());
        DFG_ASSERT_CORRECTNESS(m_data.size() <= NumericTraits<size_type>::maxValue);
        DFG_ASSERT_CORRECTNESS(m_keyToStringDetails.size() <= NumericTraits<size_type>::maxValue);
        if (m_keyToStringDetails.size() >= NumericTraits<size_type>::maxValue)
            return;
        auto nStartPos = m_data.size();
        auto nEndPos = m_data.size();
        const auto nInsertCount = static_cast<std::size_t>(std::distance(sv.beginRaw(), sv.endRaw()));
        if (nInsertCount == 0 && AreViewsNullTerminated::value)
        {
            // Optimization: using shared null for empty strings.
            nStartPos = 0;
            nEndPos = 0;
        }
        else
        {
            // Checking that size_type won't overflow in m_data if adding (i.e. that there would be indexes > maximum of size_type)
            if (nInsertCount > static_cast<size_type>(NumericTraits<size_type>::maxValue - static_cast<size_type>(nStartPos)))
                return;
            m_data.insert(m_data.end(), sv.beginRaw(), sv.endRaw());
            nEndPos = m_data.size();
            handleNullTerminator(std::integral_constant<bool, StringStorageType_T != StringStorageType::sizeOnly>());
        }
        m_keyToStringDetails[key] = StringDetail(static_cast<size_type>(nStartPos), static_cast<size_type>(nEndPos));
    }

    // Returns view at i or empty view. Note that insert() or other operation may render view invalid.
    // Note: Does not return a reference so can't be used for inserts, i.e. m[1] = "abc"; can't be used.
    // Dev note: could also optionally provide stable views based on 'this' and index to m_data, such would not invalidate e.g. on reallocations.
    const StringViewRv operator[](const Key_T& i) const
    {
        DFG_ASSERT_UB(!m_data.empty() && m_data.front() == '\0');
        auto iter = m_keyToStringDetails.find(i);
        if (iter == m_keyToStringDetails.end())
            return StringViewRv(SzPtrT(&m_data.front()));
        return iter->second.toView(m_data);
    }

    bool empty() const
    {
        return m_keyToStringDetails.empty();
    }

    size_type size() const
    {
        DFG_ASSERT_CORRECTNESS(m_keyToStringDetails.size() <= NumericTraits<size_type>::maxValue);
        return static_cast<size_type>(m_keyToStringDetails.size());
    }

    // Clears content but does not deallocate existing storages.
    void clear_noDealloc()
    {
        m_keyToStringDetails.clear();
        m_data.clear();
        m_data.push_back('\0');
    }

    // Returns the number of characters stored in content storage.
    size_t contentStorageSize() const
    {
        return this->m_data.size();
    }

    // Returns the number of base characters that storage can hold with current allocation(s). Note that by default some of the capacity is used for controls such as null terminators,
    // For example if this returns 8, it doesn't by default mean that map can store "abcd" and "efgh" without content storage reallocation(s).
    size_t contentStorageCapacity() const
    {
        return this->m_data.capacity();
    }

    // Tries to set content storage capacity and returns value effective after this call. Never shrinks capacity.
    // See documentation of contentStorageCapacity() for details.
    size_t reserveContentStorage_byBaseCharCount(const size_t nNewBaseCharCapacity)
    {
        this->m_data.reserve(nNewBaseCharCapacity); // Note: reserve() doesn't nothing if nNewBaseCharCapacity <= capacity()
        return this->contentStorageCapacity();
    }

    // Returns pointer to key corresponding to value 'sv', nullptr given value is not found. If there are multiple values matching to 'sv',
    // it is unspecified which key of mathing value is returned.
    const Key_T* keyByValue(const StringView& sv) const
    {
        for (const auto& item : *this)
        {
            if (item.second(*this) == sv)
                return &item.first;
        }
        return nullptr;
    }

    // Convenience overload: if given string is present in map, returns *keyByValue(sv), otherwise returns given default value 'returnValueIfNotFound'
    Key_T keyByValue(const StringView& sv, const Key_T& returnValueIfNotFound) const
    {
        auto p = keyByValue(sv);
        return (p) ? *p : returnValueIfNotFound;
    }

private:
    // For non-Sz view no need to add null terminator
    void handleNullTerminator(std::false_type) { }

    // For SzView adding null terminator.
    void handleNullTerminator(std::true_type)
    {
        m_data.push_back('\0');
    }

public:

    KeyStorage m_keyToStringDetails;
    DataStorage m_data; // In order to support empty string optimization, first item is always null-character. Strictly needed only for szSized-type, but done for all at the moment.
}; // class MapToStringViews

} } // module namespace
