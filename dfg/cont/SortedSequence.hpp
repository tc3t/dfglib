#pragma once

#include "../dfgDefs.hpp"
#include "elementType.hpp"
#include "../alg/sortSingleItem.hpp"
#include <algorithm>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    template <class Cont_T>
    class DFG_CLASS_NAME(SortedSequence)
    {
    public:
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueT;
        typedef typename Cont_T::size_type size_type;
        typedef typename Cont_T::iterator contTIterator;
        typedef typename Cont_T::const_iterator contTConstIterator;
        typedef contTConstIterator const_iterator;
        typedef typename Cont_T::value_type value_type;

        void insert(ValueT val)
        {
            m_cont.push_back(val);
            ::DFG_MODULE_NS(alg)::sortSingleItem(m_cont, m_cont.end());
        }

        void reserve(const size_type n)
        {
            m_cont.reserve(n);
        }

        // TODO: test
        void replace(const value_type& old, const value_type& newVal)
        {
            auto iter = findEditable(old);
            if (iter == m_cont.end())
                return;
            *iter = newVal;
            ::DFG_MODULE_NS(alg)::sortSingleItem(m_cont, iter);
        }

        // TODO: test
        void erase(const value_type& val)
        {
            auto iter = findEditable(val);
            if (iter != m_cont.end())
                m_cont.erase(iter);
        }

        bool empty() const { return m_cont.empty(); }
        size_type size() const { return m_cont.size(); }

        const_iterator begin()  const   { return m_cont.begin();  }
        const_iterator cbegin() const   { return m_cont.cbegin(); }
        const_iterator end()    const   { return m_cont.end();    }
        const_iterator cend()   const   { return m_cont.cend();   }

        contTConstIterator find(const value_type& val) const
        {
            return findImpl<contTConstIterator>(*this, val);
        }

    private:
        template <class IterT, class This_T>
        static IterT findImpl(This_T&& rThis, const value_type& val)
        {
            auto& cont = rThis.m_cont;
            auto iter = std::lower_bound(cont.begin(), cont.end(), val);
            return (iter != cont.end() && *iter == val) ? iter : cont.end();
        }

        contTIterator findEditable(const value_type& val)
        {
            return findImpl<contTIterator>(*this, val);
        }

    public:
        Cont_T m_cont;
    };

} } // module namespace
