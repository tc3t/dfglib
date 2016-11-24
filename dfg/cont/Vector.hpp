#pragma once

#include "../dfgDefs.hpp"
#include "../baseConstructorDelegate.hpp"
#include "../typeTraits.hpp"
#include <cstring>
#include <vector>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    template <class Value_T>
    class DFG_CLASS_NAME(Vector) : public std::vector<Value_T> // Note: inheritance is implementation detail (i.e. don't use it)
    {
    public:
        typedef std::vector<Value_T> BaseClass;
        typedef typename BaseClass::iterator        iterator;
        typedef typename BaseClass::const_iterator  const_iterator;
        typedef typename BaseClass::value_type      value_type;

        DFG_CLASS_NAME(Vector)() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(Vector), BaseClass) {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_2(DFG_CLASS_NAME(Vector), BaseClass) {}

        template <class Cont_T, class Iter_T, class Val_T>
        static typename Cont_T::iterator insertWithMemMove(Cont_T& cont, Iter_T pos, Val_T&& value)
        {
            DFG_STATIC_ASSERT(DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyable<value_type>::value, "Non memcpy'able type given to insertWithMemMove()");
            const auto nAddTo = pos - cont.begin();
            const auto nMoveCount = cont.size() - nAddTo;
            cont.push_back(typename Cont_T::value_type());
            std::memmove(cont.data() + nAddTo + 1, cont.data() + nAddTo, sizeof(typename Cont_T::value_type) * nMoveCount);
            cont[nAddTo] = value;
            return cont.begin() + nAddTo;
        }

        // Implements insert() for types which are not for sure known to be memcpy-safe.
        template <class Iter_T, class Val_T>
        iterator insertImpl(Iter_T pos, Val_T&& value, DFG_MODULE_NS(TypeTraits)::ConstructibleFromAnyType)
        {
            return BaseClass::insert(pos, std::forward<Val_T>(value));
        }

        // Implements memmove-insert if is_trivially_copyable<value_type> == true.
        template <class Iter_T, class Val_T>
        iterator insertImpl(Iter_T pos, Val_T&& value, std::true_type)
        {
            return insertWithMemMove(*this, pos, std::forward<Val_T>(value));
        }

        template <class Iter_T, class Val_T>
        iterator insert(Iter_T iter, Val_T&& val) { return insertImpl(iter, std::forward<Val_T>(val), DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyable<value_type>()); }
    };

} } // module namespace
