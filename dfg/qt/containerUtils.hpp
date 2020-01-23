#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "../build/languageFeatureInfo.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QObject>
#include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

#include <memory>
#include <cstddef>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

/*
Provides owning smart pointer storage for object of QObject-inherited class.
This storage makes sure that object gets deleted even if it does not have a parent and guards against double deletion
in the case that owned object gets deleted earlier e.g. due it's parent getting deleted.

Rationale:
    Q: Why not simply have "T* m_member;" in class and control deletion through parentship?
    A: Raw pointers require documentation about ownership and will leak if one forgets to set parent - and nothing enforces parenting.

    Q: Why not "std::unique_ptr<T> m_spMember;"? There's no harm deleting before parent does deleting.
    A: This may work, but if e.g. moving a target widget under another child widget, deletion of the child widget may happen before std::unique_ptr
       deletes the target widget in case which double deletion would occur. While this could be avoided by handling deletion order correctly
       e.g. by correct member ordering, this seems obscure and error prone.
TODO: test
*/
template <class T>
class QObjectStorage
{
public:
    QObjectStorage(T* newItem = nullptr)
    {
        m_spData = newItem;
    }

#if DFG_LANGFEAT_HAS_DEFAULTED_AND_DELETED_FUNCTIONS == 1
    QObjectStorage(const QObjectStorage&) = delete;
    QObjectStorage& operator=(const QObjectStorage&) = delete;
#else // case: compiler has no '= delete'
    DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(QObjectStorage);
public:
#endif
    QObjectStorage(QObjectStorage&& other)
    {
        *this = std::move(other);
    }

    ~QObjectStorage()
    {
        auto p = m_spData.data();
        m_spData.clear();
        delete p;
    }

    T& operator*()              { return *m_spData; }
    const T& operator*() const  { return *m_spData; }

    T* operator->()             { return m_spData.data(); }
    const T* operator->() const { return m_spData.data(); }

    void reset(T* newItem)
    {
        if (m_spData.data())
        {
            delete m_spData.data();
            m_spData.clear();
        }
        m_spData = QPointer<T>(newItem);
    }

    operator bool() const
    {
        return m_spData.data() != nullptr;
    }

    T* get()
    {
        return m_spData.data();
    }

    T* data()
    {
        return get();
    }

    T* release()
    {
        auto old = m_spData.data();
        m_spData.clear();
        return old;
    }

    bool operator!=(const std::nullptr_t) const
    {
        return m_spData.data() != nullptr;
    }

    QObjectStorage& operator=(QObjectStorage&& other)
    {
        reset(other.release());
        return *this;
    }

    QObjectStorage<T>& operator=(std::unique_ptr<T> other)
    {
        this->m_spData = other.release();
        return *this;
    }

    QPointer<T> m_spData;
};


}} // Module namespace
