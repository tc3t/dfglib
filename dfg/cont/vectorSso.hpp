#pragma once

#include "../dfgDefs.hpp"
#include "../cont/contAlg.hpp"
#include <type_traits>


DFG_ROOT_NS_BEGIN { DFG_SUB_NS(cont) {

// Placeholder for properly implemented SSO-vector (=vector with internal buffer).
// NOTE: this is a very quick draft with loads of missing functionality and restricting assumptions so use with caution.
// Related implementations:
//    -LLVM_SmallVector
//    -QVarLengthArray
template <class T, size_t StaticSize_T>
class VectorSso
{
public:
    typedef T*          iterator;
    typedef const T*    const_iterator;
    typedef T           value_type;
    typedef const T&    const_reference;

    enum { s_ssoBufferSize = StaticSize_T };

    VectorSso()
        : m_pData(m_ssoStorage)
        , m_nSize(0)
    {
    }

    VectorSso(const VectorSso&)            = delete;
    VectorSso(VectorSso&&)                 = delete;
    VectorSso& operator=(const VectorSso&) = delete;
    VectorSso& operator=(VectorSso&&)      = delete;

    bool isSsoStorageInUse() const
    {
        return m_pData == m_ssoStorage;
    }

    void push_back(const T& c)
    {
        bool bUseLargeStorage = false;
        if (isSsoStorageInUse())
        {
            if (m_nSize < s_ssoBufferSize)
                m_pData[m_nSize++] = c;
            else // Growing out of SSO-storage -> copy existing data to large storage.
            {
                bUseLargeStorage = true;
                m_largeStorage.reserve(m_nSize + 1);
                m_largeStorage.resize(m_nSize);
                std::move(begin(), end(), m_largeStorage.begin());
            }
        }
        else
            bUseLargeStorage = true;

        if (bUseLargeStorage)
        {
            m_largeStorage.push_back(c);
            m_nSize++;
            m_pData = m_largeStorage.data();
        }
    }

    bool empty() const
    {
        return m_nSize == 0;
    }

    void clear()
    {
        m_nSize = 0;
        if (!isSsoStorageInUse())
            m_largeStorage.clear();
        m_pData = m_ssoStorage; // Start using sso-storage on clear().
    }

    size_t size() const { return m_nSize; }

    iterator        begin()         { return m_pData; }
    const_iterator  begin() const   { return m_pData; }
    const_iterator  cbegin() const  { return begin(); }

    iterator        end()           { return begin() + m_nSize; }
    const_iterator  end() const     { return begin() + m_nSize; }
    const_iterator  cend() const    { return end(); }

          T* data()       { return begin(); }
    const T* data() const { return begin(); }

    const T& front() const
    {
        DFG_ASSERT_UB(!empty());
        return *begin();
    }

    const T& back() const
    {
        DFG_ASSERT_UB(!empty());
        return m_pData[m_nSize - 1];
    }

    void pop_front()
    {
        DFG_ASSERT_UB(!empty());
        if (isSsoStorageInUse())
        {
            DFG_STATIC_ASSERT(std::is_trivially_destructible<T>::value, "VectorSso: Implementation assumes trivially destructible types");
            std::move(begin() + 1, end(), begin()); // Moving left is ok.
            --m_nSize;
        }
        else
        {
            popFront(m_largeStorage);
            m_pData = m_largeStorage.data();
            m_nSize--;
            DFG_ASSERT_UB(m_nSize == m_largeStorage.size());
        }
    }

    void pop_back()
    {
        DFG_ASSERT_UB(!empty());
        DFG_STATIC_ASSERT(std::is_trivially_destructible<T>::value, "VectorSso: Implementation assumes trivially destructible types");
        m_nSize--;
        if (!isSsoStorageInUse())
        {
            m_largeStorage.pop_back();
            DFG_ASSERT_UB(m_nSize == m_largeStorage.size());
        }
    }

    void cutTail(const_iterator iter)
    {
        if (isSsoStorageInUse())
        {
            DFG_STATIC_ASSERT(std::is_trivially_destructible<T>::value, "VectorSso: Implementation assumes trivially destructible types");
            m_nSize = iter - begin();
        }
        else
        {
            DFG_MODULE_NS(cont)::cutTail(m_largeStorage, m_largeStorage.begin() + (iter - m_pData));
            m_nSize = m_largeStorage.size();
        }
    }

    const T& operator[](const size_t i) const
    {
        DFG_ASSERT_UB(i < size());
        return m_pData[i];
    }

    T* m_pData;
    T m_ssoStorage[s_ssoBufferSize];
    size_t m_nSize;
    std::vector<T> m_largeStorage;
}; // VectorSso

}} // module namespace
