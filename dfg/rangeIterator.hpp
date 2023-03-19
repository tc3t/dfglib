#pragma once

#include "dfgDefs.hpp"
#include <iterator> // For std::distance
#include "iter/IsContiguousMemoryIterator.hpp"
#include "dfgAssert.hpp"
#include "numericTypeTools.hpp"
#include "iter/FunctionValueIterator.hpp"

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
    class RangeIterator_T : public std::conditional<IsContiguousMemoryIterator<Iter_T>::value, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<Iter_T>, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorDefaultBase<Iter_T>>::type
    {
    public:
        typedef typename std::conditional<IsContiguousMemoryIterator<Iter_T>::value, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<Iter_T>, ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorDefaultBase<Iter_T>>::type BaseClass;
        RangeIterator_T() {}
        RangeIterator_T(Iter_T iBegin, Iter_T iEnd) : BaseClass(iBegin, iEnd) {}

        template <class Iterable_T>
        RangeIterator_T(Iterable_T&& iterable) :
            RangeIterator_T(std::begin(iterable), std::end(iterable))
        {}

        template <class OtherIter_T>
        RangeIterator_T(const RangeIterator_T<OtherIter_T>& other)
            : BaseClass(::DFG_ROOT_NS::DFG_DETAIL_NS::makeBeginIterator<Iter_T>(other.begin(), other.end()), ::DFG_ROOT_NS::DFG_DETAIL_NS::makeEndIterator<Iter_T>(other.begin(), other.end()))
        {
        }
    };

    // Specialization for pointer types
    template <class T>
    class RangeIterator_T<T*> : public ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<T*>
    {
    public:
        using BaseClass = ::DFG_ROOT_NS::DFG_DETAIL_NS::RangeIteratorContiguousIterBase<T*>;
        RangeIterator_T() {}
        RangeIterator_T(T* pBegin, T* pEnd) : BaseClass(pBegin, pEnd) {}

        template <class Iterable_T>
        RangeIterator_T(Iterable_T&& iterable) :
            RangeIterator_T(iterable.data(), iterable.data() + iterable.size())
        {}

        template <class OtherIter_T>
        RangeIterator_T(const RangeIterator_T<OtherIter_T>& other)
            : BaseClass(::DFG_ROOT_NS::DFG_DETAIL_NS::makeBeginIterator<T*>(other.begin(), other.end()), ::DFG_ROOT_NS::DFG_DETAIL_NS::makeEndIterator<T*>(other.begin(), other.end()))
        {
        }
    };

    template <class Iter_T>
    RangeIterator_T<Iter_T> makeRange(Iter_T iBegin, Iter_T iEnd)
    {
        return RangeIterator_T<Iter_T>(iBegin, iEnd);
    }

    template <class T, size_t N>
    RangeIterator_T<T*> makeRange(T(&arr)[N])
    {
        return makeRange(std::begin(arr), std::end(arr));
    }

    template <class BeginEndInterface_T>
    auto makeRange(BeginEndInterface_T& bei) -> RangeIterator_T<decltype(std::begin(bei))>
    {
        return makeRange(std::begin(bei), std::end(bei));
    }

    // Convenience function for creating range from [iterable.begin() + nStart, iterable.begin() + nEnd[
    // Preconditions:
    //      -nStart and nEnd must be within [0, iterable.size()] 
    //      -nStart must be less or equal to nEnd 
    template <class Iterable_T>
    auto makeRangeFromStartAndEndIndex(Iterable_T&& iterable, const size_t nStart, const size_t nEnd) -> decltype(makeRange(iterable))
    {
        DFG_ASSERT_UB(nStart <= nEnd && nEnd <= iterable.size());
        return makeRange(std::begin(iterable) + nStart, std::begin(iterable) + nEnd);
    }

    // Convenience method for creating range from data() guaranteeing that resulting range
    // is pointer-based.
    template <class Iterable_T>
    auto makeRange_data(Iterable_T&& iterable) -> RangeIterator_T<decltype(iterable.data())>
    {
        return makeRange(iterable.data(), iterable.data() + iterable.size());
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

    // Makes index range [first, onePastLast[. Behaviour is undefined if not (first <= onePastLast)
    template <class Index_T>
    auto indexRangeIE(const Index_T first, const Index_T onePastLast) -> decltype(makeRange(::DFG_MODULE_NS(iter)::makeIndexIterator(Index_T()), ::DFG_MODULE_NS(iter)::makeIndexIterator(Index_T())))
    {
        using namespace ::DFG_MODULE_NS(iter);
        DFG_ASSERT_UB(first <= onePastLast);
        return makeRange(makeIndexIterator(first), makeIndexIterator(onePastLast));
    }
} // namespace DFG_ROOT_NS
