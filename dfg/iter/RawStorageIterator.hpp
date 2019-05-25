#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(iter) {

// Iterator for iterating raw bytes as if it was storage of type T.
// In particular this class is intended to handle:
//      -Alignment
//      -Peculiarities of C++ in low-level object manipulation (see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0593r3.html)
template <class T>
class RawStorageIterator
{
public:
    typedef typename std::conditional<std::is_const<T>::value, const char*, char*>::type InternalIterator;

    RawStorageIterator(InternalIterator p) :
        m_p(p)
    {}

    // Note: returns a copy, not a modifiable reference.
    const T operator*() const
    {
        typename std::remove_const<T>::type a;
        memcpy(&a, m_p, sizeof(T));
        return a;
    }

    RawStorageIterator& operator++()
    {
        m_p += sizeof(T);
        return *this;
    }

    RawStorageIterator operator++(int)
    {
        auto iter = *this;
        ++*this;
        return iter;
    }

    bool operator==(const RawStorageIterator& other) const
    {
        return this->m_p == other.m_p;
    }

    bool operator!=(const RawStorageIterator& other) const
    {
        return !(*this == other);
    }

    InternalIterator ptrChar() const
    {
        return m_p;
    }

    InternalIterator m_p;
};

} } // module namespace
