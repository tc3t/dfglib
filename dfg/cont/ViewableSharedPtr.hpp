#pragma once

#include "../dfgDefs.hpp"
#include "../build/languageFeatureInfo.hpp"
#include "SetVector.hpp"

#if DFG_LANGFEAT_MUTEX_11

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS // VC2012 generates C4265 "class has virtual functions, but destructor is not virtual" from <mutex>.
    #include <mutex>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#include <map>
#include <functional>
#include <memory>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    class SourceResetParam {};
    typedef std::function<void(SourceResetParam)> SourceResetNotifier;
    typedef const void* SourceResetNotifierId;

    template <class T> class ViewableSharedPtr;

    namespace DFG_DETAIL_NS
    {
        // Maps viewer to object. All viewers refer to the same proxy object so when source get's reset, updating the single router object updates view for all viewers.
        template <class T>
        class ViewableSharedPtrRouterProxy
        {
        public:
            typedef std::map<SourceResetNotifierId, SourceResetNotifier> SourceResetNotifierMap;
            typedef std::mutex MutexT;
            typedef std::lock_guard<MutexT> LockGuardT;
            typedef ViewableSharedPtr<T> ViewableSharedPtrT;

            ViewableSharedPtrRouterProxy(ViewableSharedPtrT& rParent)
                : m_pObj(&rParent)
            {}

            ~ViewableSharedPtrRouterProxy()
            {
            }

            void onViewableReset()
            {
                LockGuardT lock(m_mutex);
                for (auto& notifierItem : m_notifiers)
                {
                    notifierItem.second(SourceResetParam());
                }
            }

            void onResourceOwnerBeingDestroyed()
            {
                LockGuardT lock(m_mutex);
                DFG_ASSERT_CORRECTNESS(this->m_notifiers.empty()); // Owner is expected to reset() before calling onResourceOwnerBeingDestroyed().
                m_pObj = nullptr;
            }

            auto view() const -> std::shared_ptr<const T>;

            void addResetNotifier(SourceResetNotifierId id, SourceResetNotifier srn)
            {
                LockGuardT lock(m_mutex);
                m_notifiers[id] = srn;
            }

            void removeResetNotifier(SourceResetNotifierId id)
            {
                LockGuardT lock(m_mutex);
                m_notifiers.erase(id);
            }

            size_t getResetNotifierCount() const
            {
                LockGuardT lock(m_mutex);
                return m_notifiers.size();
            }

            void addViewer(const void* p)
            {
                LockGuardT lock(m_mutex);
                m_viewers.insert(p);
            }

            void removeViewer(const void* p)
            {
                LockGuardT lock(m_mutex);
                m_viewers.erase(p);
            }

            size_t getViewerCount() const
            {
                LockGuardT lock(m_mutex);
                return m_viewers.size();
            }

            ViewableSharedPtrT* m_pObj;
            SourceResetNotifierMap m_notifiers;
            SetVector<const void*> m_viewers;
            mutable MutexT m_mutex;
        };
    } // namespace DFG_DETAIL_NS

    // A viewer object that can be used to create a view to object owned by ViewableSharedPtr
    // Thread safety: not thread-safe in the sense that using the same viewer-object from different threads is not safe,
    //                but different ViewableSharedPtrViewer's can safely create views to the same resource from different threads.
    template <class T>
    class ViewableSharedPtrViewer
    {
    public:
        typedef DFG_DETAIL_NS::ViewableSharedPtrRouterProxy<T> RouterT;

        ViewableSharedPtrViewer(std::shared_ptr<RouterT> spRouter = nullptr) :
            m_spRouter(std::move(spRouter))
        {
            if (m_spRouter)
                m_spRouter->addViewer(this);
        }

        ~ViewableSharedPtrViewer()
        {
            reset();
        }

        // Returns true if this viewer is not attached to any router. Note that even if attached to a router, it might have no object.
        bool isNull() const
        {
            return m_spRouter.get() == nullptr;
        }

        // Removes association of 'this' to any data, after this isNull() returns true.
        void reset()
        {
            if (m_spRouter)
            {
                m_spRouter->removeResetNotifier(this);
                m_spRouter->removeViewer(this);
                m_spRouter.reset();
            }
        }

        // Returns shared_ptr to viewed object or empty.
        std::shared_ptr<const T> view() const
        {
            return (m_spRouter) ? m_spRouter->view() : std::shared_ptr<const T>();
        }

        std::shared_ptr<RouterT> m_spRouter;
    }; // class ViewableSharedPtrViewer


    /*
    Wrapper for shared_ptr<T> with the following properties:
        -Can create viewers that are automatically and in thread safe manner updated to point to the new object if shared object in 'this' gets reset.
            -When object is changed, any existing views that viewers have created remain valid pointing to the old object, and all newly created views will point to new object.
        -Can edit() owned resource in thread safe manner with respect to viewers.
        -Is thread-safe: different threads can call even non-const functions of the same ViewableSharedPtr object.
    */
    template <class T>
    class ViewableSharedPtr
    {
    public:
        typedef DFG_DETAIL_NS::ViewableSharedPtrRouterProxy<T> RouterT;
        typedef std::mutex MutexT; // TODO: should be std::shared_mutex in order implement readwrite locking, shared_mutex is available since C++17 (https://en.cppreference.com/w/cpp/thread/shared_mutex)
        typedef std::unique_lock<MutexT> LockGuardT;

        ViewableSharedPtr(const ViewableSharedPtr&) = delete;
        ViewableSharedPtr& operator=(const ViewableSharedPtr&) = delete;

        ViewableSharedPtr(std::shared_ptr<T> sp = std::shared_ptr<T>()) :
            m_spObj(std::move(sp))
        {
            m_spRouter = std::make_shared<RouterT>(*this);
        }

        ViewableSharedPtr(ViewableSharedPtr&& other)
        {
            LockGuardT lock(other.m_mutex);
            m_spObj = std::move(other.m_spObj);
            m_spRouter = std::move(other.m_spRouter);
        }

        ~ViewableSharedPtr()
        {
            reset(std::shared_ptr<T>());
            m_spRouter->onResourceOwnerBeingDestroyed();
        }

        // Creates viewer.
        ViewableSharedPtrViewer<T> createViewer()
        {
            return ViewableSharedPtrViewer<T>(m_spRouter);
        }

        void reset(std::shared_ptr<T> other = std::shared_ptr<T>())
        {
            LockGuardT lock(m_mutex);

            m_spObj = std::move(other);
            m_spRouter->onViewableReset();
        }

        void addResetNotifier(SourceResetNotifierId id, SourceResetNotifier srn)
        {
            if (m_spRouter)
                m_spRouter->addResetNotifier(id, srn);
        }

        void removeResetNotifier(SourceResetNotifierId id)
        {
            if (m_spRouter)
                m_spRouter->removeResetNotifier(id);
        }

        // Returns copy of the underlying shared_ptr.
        // Note: this is a low-level function returning non-const T and thus bypassing the normal intended use logic: if needed, to be used with care.
        std::shared_ptr<T> sharedPtrCopy()
        {
            LockGuardT lock(m_mutex);
            return m_spObj;
        }

        const T* get() const
        {
            LockGuardT lock(m_mutex);
            return m_spObj.get();
        }

        DFG_EXPLICIT_OPERATOR_BOOL_IF_SUPPORTED operator bool() const
        {
            LockGuardT lock(m_mutex);
            return m_spObj != nullptr;
        }

        // Edits owned object in thread safe manner: if there are no views (use_count() of object is 1), edits object directly blocking view creation while editor is active.
        // If there are views, creates new object and reset()'s it in place when editor is done.
        // Editor receives two parameters: T to edit and optionally old read-only item (in case that T is newly constructed).
        void edit(std::function<void(T&, const T*)> editor)
        {
            if (!editor)
                return;

            LockGuardT lock(m_mutex);
            if (m_spObj && getReferrerCount(lock) == 0)
            {
                // In this case there are no viewers -> editing existing object. During this new views can't be created from viewers.
                editor(*m_spObj, nullptr);
            }
            else // Case: have at least one view -> can't edit concurrently -> must create a new object.
            {
                lock.unlock(); // Since we're not editing the existing object, don't need to hold the lock anymore -> viewers will be able to obtain view to old data until reset() below.
                auto spNew = std::make_shared<T>(); // TODO: shouldn't require default constructibility from T
                auto viewer = createViewer();
                editor(*spNew, viewer.view().get());
                reset(std::move(spNew));
            }
        }

        // Returns count of active referrers. Note that if there are multiple threads accessing *this (e.g. through viewers), the result may already be out-of-date when received.
        size_t getReferrerCount() const
        {
            LockGuardT lock(m_mutex);
            return (m_spObj) ? getReferrerCount(lock) : 0;
        }

        // If unable to lock, returns empty.
        std::shared_ptr<const T> privCreateView() const
        {
            LockGuardT lockGuard(m_mutex, std::try_to_lock_t());
            if (lockGuard.owns_lock())
                return m_spObj;
            else
                return std::shared_ptr<const T>();
        }

    private:
        // Requires:
        //      -m_spObj != nullptr
        //      -mutex has been locked before calling this function.
        // Note that while generally std::shared_ptr::use_count() is not reliable determining whether object can be edited
        // (e.g. one thread might check use_count() == 1 and start editing, but another thread would concurrently create a new instance e.g. through std::weak_ptr::lock()),
        // with ViewableSharedPtr referrers should be created through viewer-objects, which in turn use mutex protected privCreateView() in this class for actual implementation.
        // With these constrains use_count() seems sufficient for determining if owned object can be edited inplace.
        size_t getReferrerCount(LockGuardT&) const
        {
            return m_spObj.use_count() - 1; // Minus one to exclude reference by *this.
        }

    public:

        std::shared_ptr<T> m_spObj;
        std::shared_ptr<RouterT> m_spRouter;
        mutable std::mutex m_mutex; // Protects accesses to m_spObj. Note that m_spRouter does not need locking as it has it's own:
                                    //       all accesses of m_spRouter in 'this' (excluding move constructor) are through const member functions (e.g. std::shared_ptr<T>::operator->() is const)
                                    //       so for example can copy m_spRouter to constructor of ViewableSharedPtrViewer-object in createViewer()
                                    //       even if other threads are e.g. calling this->addResetNotifier() or even this->reset() 
    };

    template <class T>
    auto DFG_DETAIL_NS::ViewableSharedPtrRouterProxy<T>::view() const ->  std::shared_ptr<const T>
    {
        LockGuardT lock(m_mutex);
        return (m_pObj) ? m_pObj->privCreateView() : nullptr;
    }

} } // module namespace

#endif // DFG_LANGFEAT_MUTEX_11
