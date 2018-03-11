#pragma once

#include "../dfgDefs.hpp"
#include <type_traits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

namespace DFG_DETAIL_NS
{

    // Base class for non-empty deleter.
    template <class Resource_T, class Deleter_T, bool IsEmptyDeleter>
    class UniqueResourceHolderBase
    {
    public:
        UniqueResourceHolderBase(Resource_T&& resource, Deleter_T&& deleter) :
            m_resource(std::forward<Resource_T>(resource)),
            m_deleter(std::forward<Deleter_T>(deleter))
        {}

        UniqueResourceHolderBase(UniqueResourceHolderBase&& other) :
            m_resource(std::move(other.m_resource)),
            m_deleter(std::move(other.m_deleter))
        {
            other.m_resource = Resource_T(); // TODO: make this more robust. This is ok for pointers et al, but can generate numerous problems for less trivial resource types.
        }

        Deleter_T& getDeleter() { return m_deleter; }

        Resource_T m_resource;
        Deleter_T m_deleter;
    };

    // Specialization for empty deleter
    template <class Resource_T, class Deleter_T>
    class UniqueResourceHolderBase<Resource_T, Deleter_T, true> : public Deleter_T
    {
    public:
        typedef Deleter_T BaseClass;

        UniqueResourceHolderBase(Resource_T&& resource, Deleter_T&& deleter) :
            BaseClass(std::forward<Deleter_T>(deleter)),
            m_resource(std::forward<Resource_T>(resource))
        {}

        UniqueResourceHolderBase(UniqueResourceHolderBase&& other) :
            BaseClass(std::move(other)),
            m_resource(std::move(other.m_resource))
        {
            other.m_resource = Resource_T(); // TODO: make this more robust. This is ok for pointers et al, but can generate numerous problems for less trivial resource types.
        }

        Deleter_T& getDeleter() { return *this; }

        Resource_T m_resource;
    };

} // DFG_DETAIL_NS


// Const semantics: Uses 'pointer const' semantics, i.e. can edit resource value through const UniqueResourceHolder<T>, use UniqueResourceHolder<const T> for holding const content.
template <class Resource_T, class Deleter_T>
class UniqueResourceHolder : public DFG_DETAIL_NS::UniqueResourceHolderBase<Resource_T, Deleter_T, std::is_empty<Deleter_T>::value>
{
public:
    typedef DFG_DETAIL_NS::UniqueResourceHolderBase<Resource_T, Deleter_T, std::is_empty<Deleter_T>::value> BaseClass;
    UniqueResourceHolder(Resource_T&& resource, Deleter_T&& deleter) :
        BaseClass(std::forward<Resource_T>(resource), std::forward<Deleter_T>(deleter)),
        m_active(true)
    {}

    UniqueResourceHolder(UniqueResourceHolder&& other) :
        BaseClass(std::move(other)),
        m_active(true)
    {
        other.m_active = false;
    }

    ~UniqueResourceHolder()
    {
        if (m_active)
            this->getDeleter()(this->m_resource);
    }

          Resource_T& data()             { return this->m_resource; }
    const Resource_T& data() const       { return this->m_resource; }

          Resource_T& operator->()       { return data(); }
    const Resource_T& operator->() const { return data(); }

    // TODO: add operator* for pointer types.

    DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(UniqueResourceHolder)

    bool m_active;
};

template <class Resource_T, class Deleter_T>
class UniqueResourceHolderImplicityConvertible : public UniqueResourceHolder<Resource_T, Deleter_T>
{
private:
    typedef UniqueResourceHolder<Resource_T, Deleter_T> BaseClass;
public:
    UniqueResourceHolderImplicityConvertible(Resource_T&& resource, Deleter_T&& deleter) :
        BaseClass(std::forward<Resource_T>(resource), std::forward<Deleter_T>(deleter))
    {}

    // Note: non-const implicit conversion is not allowed for a reason; for example with int*:
    // *uniqueResourceHandler = 2; // works
    // uniqueResourceHandler += 2; // if non-const conversion was allowed, this would compile and changing pointer value likely leading to crash when deleter is executed.
    operator const Resource_T& () const    { return this->data(); }
};

template <class Resource_T, class Deleter_T>
auto makeUniqueResourceHolder_explicitlyConvertible(Resource_T&& resource, Deleter_T&& deleter) -> UniqueResourceHolder<Resource_T, Deleter_T>
{
    return UniqueResourceHolder<Resource_T, Deleter_T>(std::forward<Resource_T>(resource), std::forward<Deleter_T>(deleter));
}

template <class Resource_T, class Deleter_T>
auto makeUniqueResourceHolder_implicitlyConvertible(Resource_T&& resource, Deleter_T&& deleter) -> UniqueResourceHolderImplicityConvertible<Resource_T, Deleter_T>
{
    return UniqueResourceHolderImplicityConvertible<Resource_T, Deleter_T>(std::forward<Resource_T>(resource), std::forward<Deleter_T>(deleter));
}

} } // module namespace
