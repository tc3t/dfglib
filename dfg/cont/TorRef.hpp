#pragma once

#include "../dfgDefs.hpp"
#include <dfg/baseConstructorDelegate.hpp>
#include <memory>
#include <array>
#include "../dfgAssert.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    namespace DFG_DETAIL_NS
    {
        template <class Impl_T, class T>
        class TorRefInternalStorageCRTP
        {
        public:
            bool hasItem() const { return static_cast<const Impl_T&>(*this).hasItem(); }
            void createDefaultItem() { static_cast<Impl_T&>(*this).createDefaultItem(); }
            void setItem(const T& src) { static_cast<Impl_T&>(*this).setItem(src); }
            T* itemPtr() const { return static_cast<const Impl_T&>(*this).itemPtr(); }
        };

        // TODO: this class should guarantee that it does not do any direct heap allocations since the internal object is stored within the storage object.
        template <class T>
        class TorRefInternalStorageStack : public TorRefInternalStorageCRTP<TorRefInternalStorageStack<T>, T>
        {
        public:
            typedef std::array<T, 1> StorageType; // TODO: use storage which constructs instance of T only when needed (see vectorFixedCapacity.hpp)

            TorRefInternalStorageStack()
            {
                m_item[0] = T(); // Must initialize because hasItem() always returns true with current implementation.
            }
            bool hasItem() const        { return !m_item.empty(); }
            void createDefaultItem()    { m_item[0] = T(); }
            void setItem(const T& src)  { m_item[0] = src; }
            T*        itemPtr()         { return &m_item[0]; }
            const T*  itemPtr() const   { return &m_item[0]; }

            const StorageType& storage() const { return m_item; }

            StorageType m_item;
        };

        template <class T>
        class TorRefInternalStorageHeap : public TorRefInternalStorageCRTP<TorRefInternalStorageHeap<T>, T>
        {
        public:
            typedef std::unique_ptr<T> StorageType;

            TorRefInternalStorageHeap() {}
            TorRefInternalStorageHeap(TorRefInternalStorageHeap&& other) :
                m_spItem(std::move(other.m_spItem))
            {}
            bool hasItem() const                { return m_spItem.get() != nullptr; }
            void createDefaultItem()            { m_spItem.reset(new T()); }
            void setItem(const T& src)          { if (!m_spItem) m_spItem.reset(new T(src)); else *m_spItem = src; }
            T* itemPtr() const                  { return m_spItem.get(); }
            const StorageType& storage() const  { return m_spItem; }

            std::unique_ptr<T> m_spItem;
        };
    } // namespace DFG_DETAIL_NS

    // Stores either reference to externally owned resource or resource itself.
    // Can be used e.g. in return values when the ownership of returned resource may vary.
    // Note: this class does not guarantee thread safety even for const accesses.
    template <class T, class InternalStorage_T = DFG_DETAIL_NS::TorRefInternalStorageHeap<typename std::remove_const<T>::type>, class Ref_T = T*>
    class DFG_CLASS_NAME(TorRef)
    {
    public:
        DFG_CLASS_NAME(TorRef)(Ref_T ref = Ref_T()) :
            m_ref(std::move(ref))
        {}

        DFG_CLASS_NAME(TorRef)(DFG_CLASS_NAME(TorRef)&& other) :
            m_ref(std::move(other.m_ref)),
            m_internal(std::move(other.m_internal))

        {}

        static DFG_CLASS_NAME(TorRef) makeInternallyOwning(const T& other)
        {
            DFG_CLASS_NAME(TorRef) tor;
            tor.internalStorage().setItem(other);
            return tor;
        }

        const Ref_T& referenceObject() const { return m_ref; }

        InternalStorage_T&       internalStorage()          { return m_internal; }
        const InternalStorage_T& internalStorage() const    { return m_internal; }

        T*       operator->()       { return &item(); }
        const T* operator->() const { return &item(); }

        operator       T&()         { return item(); }
        operator const T&() const   { return item(); }

        void setRef(Ref_T ref)
        {
            m_ref = std::move(ref);
        }

        // Templated implementation to avoid non-const/const code duplication.
        template <class This_T>
        static auto itemImpl(This_T& rThis) -> decltype(*rThis.m_ref)
        {
            if (rThis.hasRef())
                return *rThis.m_ref;
            else
            {
                if (!rThis.m_internal.hasItem())
                    rThis.m_internal.createDefaultItem();
                auto p = rThis.m_internal.itemPtr();
                DFG_ASSERT_UB(p != nullptr);
                return *p;
            }
        }

        T&       item()         { return itemImpl(*this); }
        const T& item() const   { return itemImpl(*this); }

        bool hasRef() const
        {
            return (m_ref != nullptr);
        }

        Ref_T m_ref;
        mutable InternalStorage_T m_internal; // Mutable because item() may need to create the internal object even for const-version.
    }; // class TorRef

    // TorRef with shared_ptr reference.
    template <class T, class InternalStorage_T = DFG_DETAIL_NS::TorRefInternalStorageHeap<typename std::remove_const<T>::type>>
    class DFG_CLASS_NAME(TorRefShared) : public DFG_CLASS_NAME(TorRef)<T, InternalStorage_T, std::shared_ptr<T>>
    {
    public:
        typedef DFG_CLASS_NAME(TorRef)<T, InternalStorage_T, std::shared_ptr<T>> BaseClass;
        DFG_CLASS_NAME(TorRefShared)() {}
        DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(TorRefShared), BaseClass) {}
    }; // class TorRefShared

} }
