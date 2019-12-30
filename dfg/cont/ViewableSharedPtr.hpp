#pragma once

#include "../dfgDefs.hpp"
#include "../build/languageFeatureInfo.hpp"

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

            size_t countOfResetNotifiers() const
            {
                LockGuardT lock(m_mutex);
                return m_notifiers.size();
            }

            std::weak_ptr<const T> m_spObj;
            SourceResetNotifierMap m_notifiers;
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
        }

        ~DFG_CLASS_NAME(ViewableSharedPtrViewer)()
        {
            if (m_spRouter)
                m_spRouter->removeResetNotifier(this);
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
        typedef std::mutex MutexT;
        typedef std::lock_guard<MutexT> LockGuardT;

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

        std::shared_ptr<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>> createViewer()
        {
            LockGuardT lock(m_mutex);
            auto spViewer = std::make_shared<DFG_CLASS_NAME(ViewableSharedPtrViewer)<T>>(m_spRouter);
            return spViewer;
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
            return m_spObj != nullptr;
        }

        std::shared_ptr<T> m_spObj;
        std::shared_ptr<RouterT> m_spRouter;
        mutable std::mutex m_mutex;

        DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(DFG_CLASS_NAME(ViewableSharedPtr));
    };

} } // module namespace

#endif // DFG_LANGFEAT_MUTEX_11
