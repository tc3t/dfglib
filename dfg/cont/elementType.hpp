#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

	namespace DFG_DETAIL_NS
	{
		template <class Cont_T> struct ElementTypeFromConstReferenceRemoved
		{
			typedef typename Cont_T::value_type type;
		};

		template <class T, size_t N> struct ElementTypeFromConstReferenceRemoved<T[N]> { typedef T type; };

		template <class T> struct ElementTypeFromConstReferenceRemoved<T*> { typedef T type; };
	} // namespace DFG_DETAIL_NS

	// Defines container element type as 'type'.
	// Example: std::vector<int> v; ElementType<decltype(v)>::type  (== int)
	template <class Cont_T> struct ElementType
	{ 
		typedef typename std::remove_reference<typename std::remove_cv<Cont_T>::type>::type cvRemovedType;
		typedef typename DFG_DETAIL_NS::ElementTypeFromConstReferenceRemoved<cvRemovedType>::type type;
	};

	

} } // module cont
