#pragma once

#include "dfgDefs.hpp"
#include <iterator> // For std::distance
#include "iter/IsContiguousMemoryIterator.hpp"
#include "dfgAssert.hpp"
#include "numericTypeTools.hpp"

DFG_ROOT_NS_BEGIN
{
    namespace DFG_DETAIL_NS
    {
        template <class Iter_T>
        class RangeIteratorDefaultBase
        {
        public:
            typedef Iter_T iterator;
            typedef typename std::iterator_traits<Iter_T>::value_type value_type;
            typedef typename std::iterator_traits<Iter_T>::reference reference;

            RangeIteratorDefaultBase() : m_iterBegin(Iter_T()), m_iterEnd(Iter_T()) {}
            RangeIteratorDefaultBase(Iter_T iBegin, Iter_T iEnd) : m_iterBegin(iBegin), m_iterEnd(iEnd) {}

            iterator begin()  const	{ return m_iterBegin; }
            iterator cbegin() const	{ return begin(); }
            iterator end()  const	{ return m_iterEnd; }
            iterator cend() const	{ return end(); }
            size_t size() const		{ return std::distance(begin(), end()); }
            int sizeAsInt() const   { return saturateCast<int>(size()); } // Returns saturateCast<int>(size())
            bool empty() const      { return m_iterBegin == m_iterEnd; }
                  value_type& front()       { return *begin(); } // Precondition: !empty()
            const value_type& front() const { return *begin(); } // Precondition: !empty()
                  value_type& back()        { auto i = end(); --i; return *i; } // Precondition: !empty()
            const value_type& back() const  { auto i = end(); --i; return *i; } // Precondition: !empty()

            reference operator[](const size_t i) const { DFG_ASSERT_UB(i < this->size()); auto iter = this->begin(); std::advance(iter, i); return *iter; }

            iterator m_iterBegin;
            iterator m_iterEnd;
        }; // Class RangeIteratorDefaultBase

        // Specialized base that defines data() and operator[] for contiguous range.
        template <class Iter_T>
        class RangeIteratorContiguousIterBase : public RangeIteratorDefaultBase<Iter_T>
        {
        public:
            typedef RangeIteratorDefaultBase<Iter_T> BaseClass;
            typedef typename std::iterator_traits<Iter_T>::pointer pointer;
            using reference = typename BaseClass::reference;
            RangeIteratorContiguousIterBase() {}
            RangeIteratorContiguousIterBase(Iter_T iBegin, Iter_T iEnd) : BaseClass(iBegin, iEnd) {}

            pointer beginAsPointer() const { return (!this->empty()) ? &*this->begin() : nullptr; }

            // Note: This is const given the const-semantics explained in comments for RangeIterator_T.
            // Note: Return value may differ from standard containers data() when range is empty.
            pointer data() const { return beginAsPointer(); }
        }; // Class RangeIteratorContiguousIterBase

        template <class Destination_T, class Source_T>
        Destination_T makeBeginIterator(const Source_T& iterBegin, const Source_T& iterEnd, std::true_type)
        {
            DFG_STATIC_ASSERT((std::is_pointer<Destination_T>::value && IsContiguousMemoryIterator<Source_T>::value), "Can't create T* from non-contiguous memory iterator");
            return (iterBegin != iterEnd) ? &*iterBegin : nullptr;
        }

        template <class Destination_T, class Source_T>
        Destination_T makeBeginIterator(const Source_T& iterBegin, const Source_T& /*iterEnd*/, std::false_type)
        {
            return iterBegin;
        }

        template <class Destination_T, class Source_T>
        Destination_T makeBeginIterator(const Source_T& iterBegin, const Source_T& iterEnd)
        {
            return makeBeginIterator<Destination_T>(iterBegin, iterEnd, typename std::is_pointer<Destination_T>());
        }

        template <class Destination_T, class Source_T>
        Destination_T makeEndIterator(const Source_T& iterBegin, const Source_T& iterEnd, std::true_type)
        {
            auto rv = makeBeginIterator<Destination_T>(iterBegin, iterEnd);
            rv += std::distance(iterBegin, iterEnd);
            return rv;
        }

        template <class Destination_T, class Source_T>
        Destination_T makeEndIterator(const Source_T&, const Source_T& iterEnd, std::false_type)
        {
            return iterEnd;
        }

        template <class Destination_T, class Source_T>
        Destination_T makeEndIterator(const Source_T& iterBegin, const Source_T& iterEnd)
        {
            return makeEndIterator<Destination_T>(iterBegin, iterEnd, typename std::is_pointer<Destination_T>());
        }

    } // namespace DFG_DETAIL_NS

    // Defines range through iterator pair.
    // To create instances conveniently, use makeRange.
    // Note: Range uses 'pointer const' semantics: range's constness is defined by the range it
    //       defines, not by the contents. Thus is it possible to modify underlying data through
    //       e.g. begin() iterator even through const range.
    // Remarks: For more robust implementation, see Boost.Range
    template <class Iter_T>
    class DFG_CLASS_NAME(RangeIterator_T) : public std::conditional<IsContiguousMemoryIterator<Iter_T>::value, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<Iter_T>, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorDefaultBase<Iter_T>>::type
    {
    public:
        typedef typename std::conditional<IsContiguousMemoryIterator<Iter_T>::value, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<Iter_T>, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorDefaultBase<Iter_T>>::type BaseClass;
        DFG_CLASS_NAME(RangeIterator_T)() {}
        DFG_CLASS_NAME(RangeIterator_T)(Iter_T iBegin, Iter_T iEnd) : BaseClass(iBegin, iEnd) {}

        template <class OtherIter_T>
        DFG_CLASS_NAME(RangeIterator_T)(const DFG_CLASS_NAME(RangeIterator_T)<OtherIter_T>& other)
            : BaseClass(::DFG_ROOT_NS::DFG_DETAIL_NS::makeBeginIterator<Iter_T>(other.begin(), other.end()), ::DFG_ROOT_NS::DFG_DETAIL_NS::makeEndIterator<Iter_T>(other.begin(), other.end()))
        {
        }
    };

    template <class Iter_T>
    DFG_CLASS_NAME(RangeIterator_T)<Iter_T> makeRange(Iter_T iBegin, Iter_T iEnd)
    {
        return DFG_CLASS_NAME(RangeIterator_T)<Iter_T>(iBegin, iEnd);
    }

    template <class T, size_t N>
    DFG_CLASS_NAME(RangeIterator_T)<T*> makeRange(T(&arr)[N])
    {
        return makeRange(std::begin(arr), std::end(arr));
    }

    template <class BeginEndInterface_T>
    auto makeRange(BeginEndInterface_T& bei) -> DFG_CLASS_NAME(RangeIterator_T)<decltype(std::begin(bei))>
    {
        return makeRange(std::begin(bei), std::end(bei));
    }

    // Returns head range, i.e. range [begin(), begin() + nCount] from given iterable.
    // Precondition: nCount must be within size of iterable.
    template <class Iterable>
    auto headRange(Iterable&& iterable, const ::std::ptrdiff_t nCount) -> decltype(makeRange(iterable))
    {
        typedef decltype(makeRange(iterable)) Return_T;
        auto iterBegin = ::std::begin(iterable);
        auto iterEnd = iterBegin;
        std::advance(iterEnd, nCount);
        return Return_T(iterBegin, iterEnd);
    }
}
