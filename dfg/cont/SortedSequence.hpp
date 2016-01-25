#pragma once

#include "../dfgDefs.hpp"
#include "ElementType.hpp"
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
			auto iter = find(old);
			if (iter == m_cont.end())
				return;
			*iter = newVal;
			::DFG_MODULE_NS(alg)::sortSingleItem(m_cont, iter);
		}

		// TODO: test
		void erase(const value_type& val)
		{
			auto iter = find(val);
			if (iter != m_cont.end())
				m_cont.erase(iter);
		}

        size_type size() const { return m_cont.size(); }

        contTConstIterator begin()  const   { return m_cont.begin();  }
        contTConstIterator cbegin() const   { return m_cont.cbegin(); }
        contTConstIterator end()    const   { return m_cont.end();    }
        contTConstIterator cend()   const   { return m_cont.cend();   }

	private:
		contTIterator find(const value_type& val)
		{
			auto iter = std::lower_bound(m_cont.begin(), m_cont.end(), val);
			return (iter != m_cont.end() && *iter == val) ? iter : m_cont.end();
		}

	public:
        Cont_T m_cont;
    };

} } // module namespace
