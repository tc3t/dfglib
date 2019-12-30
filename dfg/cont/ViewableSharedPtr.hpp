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

    class DFG_CLASS_NAME(SourceResetParam) {};
    typedef std::function<void(DFG_CLASS_NAME(SourceResetParam))> DFG_CLASS_NAME(SourceResetNotifier);
    typedef const void* DFG_CLASS_NAME(SourceResetNotifierId);

    namespace DFG_DETAIL_NS
    {
        // Maps viewer to object. All viewers refer to the same proxy object so when source get's reset, updating the single router object updates view for all viewers.
        template <class T>
        class ViewableSharedPtrRouterProxy
        {
        public:
            typedef std::map<DFG_CLASS_NAME(SourceResetNotifierId), DFG_CLASS_NAME(SourceResetNotifier)> SourceResetNotifierMap;
            typedef std::mutex MutexT;
            typedef std::lock_guard<MutexT> LockGuardT;

            void reset(const std::shared_ptr<const T>& newItem)
            {
                LockGuardT lock(m_mutex);
                m_spObj = newItem;
                for (auto& notifierItem : m_notifiers)
                {
                    notifierItem.second(DFG_CLASS_NAME(SourceResetParam)());
                }
            }

            std::shared_ptr<const T> view() const
            {
                LockGuardT lock(m_mutex);
                return m_spObj.lock();
            }

            void addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId) id, DFG_CLASS_NAME(SourceResetNotifier) srn)
            {
                LockGuardT lock(m_mutex);
                m_notifiers[id] = srn;
            }

            void removeResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId) id)
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

            std::weak_ptr<const T> m_spObj;
            SourceResetNotifierMap m_notifiers;
            DFG_CLASS_NAME(SetVector)<const void*> m_viewers;
            mutable MutexT m_mutex;
        };
    } // namespace DFG_DETAIL_NS

    template <class T>
    class DFG_CLASS_NAME(ViewableSharedPtrViewer)
    {
    public:
        typedef DFG_DETAIL_NS::ViewableSharedPtrRouterProxy<T> RouterT;

        DFG_CLASS_NAME(ViewableSharedPtrViewer)(std::shared_ptr<RouterT> spRouter) :
            m_spRouter(std::move(spRouter))
        {
            if (m_spRouter)
                m_spRouter->addViewer(this);
        }

        ~DFG_CLASS_NAME(ViewableSharedPtrViewer)()
        {
            if (m_spRouter)
            {
                m_spRouter->removeResetNotifier(this);
                m_spRouter->removeViewer(this);
            }
        }

        // Returns shared_ptr to viewed object or empty.
        std::shared_ptr<const T> view()
        {
            return (m_spRouter) ? m_spRouter->view() : std::shared_ptr<const T>();
        }

        std::shared_ptr<RouterT> m_spRouter;
    };


    /*
    Extended shared_ptr<T> with the following property:
        -Can create viewers that are automatically and in thread safe manner updated to point to the new object if shared object in 'this' gets reset.
    TODO: Review and test thread safety.
    */
    template <class T>
    class DFG_CLASS_NAME(ViewableSharedPtr)
    {
    public:
        typedef DFG_DETAIL_NS::ViewableSharedPtrRouterProxy<T> RouterT;
        typedef std::mutex MutexT; // TODO: should be std::shared_mutex in order implement readwrite locking, shared_mutex is available since C++17 (https://en.cppreference.com/w/cpp/thread/shared_mutex)
        typedef std::unique_lock<MutexT> LockGuardT;

        DFG_CLASS_NAME(ViewableSharedPtr)(std::shared_ptr<T> sp = std::shared_ptr<T>()) :
            m_spObj(std::move(sp))
        {
            m_spRouter = std::make_shared<RouterT>();
            m_spRouter->reset(m_spObj);
        }

        DFG_CLASS_NAME(ViewableSharedPtr)(DFG_CLASS_NAME(ViewableSharedPtr)&& other)
        {
            LockGuardT lock(other.m_mutex);
            m_spObj = std::move(other.m_spObj);
            m_spRouter = std::move(other.m_spRouter);
        }

        ~DFG_CLASS_NAME(ViewableSharedPtr)()
        {
            reset(std::shared_ptr<T>());
        }

        // Creates viewer, blocks calling thread until viewer can be created.
        std::shared_ptr<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>> createViewer()
        {
            LockGuardT lock(m_mutex);
            return createViewer_assumeLocked();
        }

        std::shared_ptr<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>> tryCreateViewer()
        {
            LockGuardT lockGuard(m_mutex, std::try_to_lock_t());
            if (lockGuard.owns_lock())
                return createViewer_assumeLocked();
            else
                return std::shared_ptr<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>>();
        }

        void reset(std::shared_ptr<T> other = std::shared_ptr<T>())
        {
            LockGuardT lock(m_mutex);

            m_spRouter->reset(other);
            m_spObj = std::move(other);
        }

        void addResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId) id, DFG_CLASS_NAME(SourceResetNotifier) srn)
        {
            LockGuardT lock(m_mutex);
            if (m_spRouter)
                m_spRouter->addResetNotifier(id, srn);
        }

        void removeResetNotifier(DFG_CLASS_NAME(SourceResetNotifierId) id)
        {
            LockGuardT lock(m_mutex);
            if (m_spRouter)
                m_spRouter->removeResetNotifier(id);
        }

        // Returns copy of the underlying shared_ptr.
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

        // Edits owned object in thread safe manner: if there are no viewers, edits object directly blocking viewer creation while editor is active.
        // If there are viewers, creates new object and reset()'s it in place when editor is done.
        // Editor receives two parameters: T to edit and optionally old read-only item (in case that T is newly constructed).
        void edit(std::function<void(T&, const T*)> editor)
        {
            if (!editor)
                return;

            LockGuardT lock(m_mutex);
            if (m_spObj && getViewerCount(lock) == 0 && m_spObj.use_count() < 2)
            {
                // In this case there are no viewers -> editing existing object. During this new viewers can't be acquired with tryCreateViewer().
                editor(*m_spObj, nullptr);
            }
            else // Case: have at least one viewer -> can't edit concurrently -> must create a new object.
            {
                lock.unlock(); // Since we're not editing the existing object, don't need to hold the lock anymore -> viewers will be able to obtain view to old data until reset() below.
                auto spNew = std::make_shared<T>(); // TODO: shouldn't require default constructibility from T
                auto spViewer = createViewer();
                editor(*spNew, (spViewer) ? spViewer->view().get() : nullptr);
                reset(std::move(spNew));
            }
        }

        // Returns current viewer count. Note that if there are multiple threads accessing *this, the result may already be out-of-date when received.
        size_t getViewerCount() const
        {
            return m_spRouter->getViewerCount();
        }

    private:
        std::shared_ptr<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>> createViewer_assumeLocked()
        {
            auto spViewer = std::make_shared<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>>(m_spRouter);
            return spViewer;
        }

        // Requires:
        //      -mutex has been locked before calling this function.
        size_t getViewerCount(LockGuardT&) const
        {
            return m_spRouter->getViewerCount();
        }

    public:

        std::shared_ptr<T> m_spObj;
        std::shared_ptr<RouterT> m_spRouter;
        mutable std::mutex m_mutex;

        DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(DFG_CLASS_NAME(ViewableSharedPtr));
    };

} } // module namespace

#endif // DFG_LANGFEAT_MUTEX_11
