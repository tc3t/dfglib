#pragma once

#include "../dfgDefs.hpp"
#include "ElementType.hpp"
#include "../alg/sortSingleItem.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    template <class Cont_T>
    class DFG_CLASS_NAME(SortedSequence)
    {
    public:
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueT;
        typedef typename Cont_T::size_type size_type;
        typedef typename Cont_T::const_iterator contTConstIterator;

        void insert(ValueT val)
        {
            m_cont.push_back(val);
            ::DFG_MODULE_NS(alg)::sortSingleItem(m_cont, m_cont.end());
        }

        void reserve(const size_type n)
        {
            m_cont.reserve(n);
        }

        size_type size() const { return m_cont.size(); }

        contTConstIterator begin()  const   { return m_cont.begin();  }
        contTConstIterator cbegin() const   { return m_cont.cbegin(); }
        contTConstIterator end()    const   { return m_cont.end();    }
        contTConstIterator cend()   const   { return m_cont.cend();   }

        Cont_T m_cont;
    };

} } // module namespace
