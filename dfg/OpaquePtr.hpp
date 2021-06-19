#pragma once

#include "dfgDefs.hpp"
#include <memory>

/*

Note: Items in this file are library internals.

Tools for unified usage of opaque ptr

Usage:

-In class definition, add DFG_OPAQUE_PTR_DECLARE();
-In cpp-file, add
    DFG_OPAQUE_PTR_DEFINE(namespace_if_needed::class_name)
    {
        // Add class definition here
    };

Accessing opaque_ptr:
    -DFG_OPAQUE_PTR(). Not guaranteed to be non-null.
    -DFG_OPAQUE_REF(). Will create object if not present and thus is not available as const-version.

Notes:
    -Opaque member can be copied if it is copyable.
    -Opaque member is always created when parent object is created, but in practice all use sites must check if the object exists.
        -Can be missing e.g. if parent object was moved somewhere.
    -Behaviour in inheritance is mostly unspecified, but opaque ptr machinery is guaranteed to work even if base class has DFG_OPAQUE_PTR_DECLARE() member
*/

DFG_ROOT_NS_BEGIN { namespace DFG_DETAIL_NS
{
    template <class T>
    class OpaquePtrMember 
    {
    public:
        OpaquePtrMember()
            : m_spData(new T)
        {}

        OpaquePtrMember(const OpaquePtrMember& other)
        {
            if (other.m_spData)
                this->m_spData.reset(new T(*other));
        }

        OpaquePtrMember(OpaquePtrMember&& other) noexcept
            : m_spData(std::move(other.m_spData))
        {
        }

        OpaquePtrMember& operator=(const OpaquePtrMember& other)
        {
            if (other.get() != nullptr)
            {
                if (this->get() != nullptr)
                    *this->get() = *other; // 'this' already has object, using operator=.
                else
                    this->m_spData.reset(new T(*other.get())); // 'this' does not have an object, creating new with copy contructor.
            }
            else
                this->m_spData.reset(); // other didn't have object, removing object from 'this'.
            return *this;
        }

        OpaquePtrMember& operator=(OpaquePtrMember&& other) noexcept
        {
            this->m_spData = std::move(other.m_spData);
            return *this;
        }

        T& getRef()
        {
            if (!m_spData)
                m_spData.reset(new T);
            return *m_spData;
        }

              T* get()       { return m_spData.get(); }
        const T* get() const { return m_spData.get(); }

        T&       operator*()       { return *m_spData; }
        const T& operator*() const { return *m_spData; }

        std::unique_ptr<T> m_spData;
    };
}} // DFG_ROOT_NS::DFG_DETAIL_NS

#define DFG_OPAQUE_PTR_DECLARE() \
    struct OpaqueData; \
    ::DFG_ROOT_NS::DFG_DETAIL_NS::OpaquePtrMember<OpaqueData> m_opaqueMember

#define DFG_OPAQUE_PTR_DEFINE(OWNER_CLASS) \
    struct OWNER_CLASS::OpaqueData

#define DFG_OPAQUE_PTR() (m_opaqueMember.get())
#define DFG_OPAQUE_REF() (m_opaqueMember.getRef())
