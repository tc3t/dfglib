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

        // Implements memmove-insert of is_trivially_copyable-types.
        iterator insertImpl(const_iterator pos, Value_T&& value, std::true_type)
        {
            const auto nAddTo = pos - this->begin();
            const auto nMoveCount = size() - nAddTo;
            this->push_back(value_type());
            std::memmove(this->data() + nAddTo + 1, this->data() + nAddTo, sizeof(value_type) * nMoveCount);
            (*this)[nAddTo] = std::move(value);
            return this->begin() + nAddTo;
        }

        // insertImpl() for case of not being sure whether type is trivially copyable -> use default implementation.
        iterator insertImpl(const_iterator pos, Value_T&& value, DFG_MODULE_NS(TypeTraits)::ConstructibleFromAnyType)
        {
            return BaseClass::insert(pos, std::move(value));
        }
        
        iterator insert(const_iterator pos, Value_T&& value)
        {
            return insertImpl(pos, std::move(value), DFG_MODULE_NS(TypeTraits)::IsTriviallyCopyable<value_type>());
        }
    };

} } // module namespace
