#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "../build/languageFeatureInfo.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QObject>
#include <QPointer>
#include <QReadWriteLock>
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
        : m_spData(newItem)
    {
    }

    QObjectStorage(const QObjectStorage&) = delete;
    QObjectStorage& operator=(const QObjectStorage&) = delete;

    QObjectStorage(QObjectStorage&& other) noexcept
    {
        *this = std::move(other);
    }

    ~QObjectStorage()
    {
        deleteData();
    }

    T& operator*()              { return *m_spData; }
    const T& operator*() const  { return *m_spData; }

    T* operator->()             { return m_spData.data(); }
    const T* operator->() const { return m_spData.data(); }

    void reset(T* newItem)
    {
        if (m_spData.data())
            deleteData();
        m_spData = QPointer<T>(newItem);
    }

    operator bool() const
    {
        return m_spData.data() != nullptr;
    }

    T*       get()       { return m_spData.data(); }
    const T* get() const { return m_spData.data(); }

    T*       data()       { return get(); }
    const T* data() const { return get(); }

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

    QObjectStorage& operator=(QObjectStorage&& other) noexcept
    {
        reset(other.release());
        return *this;
    }

    QObjectStorage<T>& operator=(std::unique_ptr<T> other)
    {
        this->m_spData = other.release();
        return *this;
    }

private:
    void deleteData()
    {
        auto p = m_spData.data();
        m_spData.clear();
        delete p;
    }

    QPointer<T> m_spData;
}; // class QObjectStorage


class LockReleaser
{
public:
    LockReleaser(QReadWriteLock* pLock = nullptr) : m_pLock(pLock) {}
    LockReleaser(LockReleaser&& other) noexcept : m_pLock(other.m_pLock) { other.m_pLock = nullptr; }
    ~LockReleaser();
    LockReleaser(const LockReleaser&) = delete;
    LockReleaser& operator=(LockReleaser&& other) noexcept;
    LockReleaser& operator=(const LockReleaser&) = delete;
    bool isLocked() const { return m_pLock != nullptr; }
    void unlock();

    QReadWriteLock* m_pLock;
}; // class LockReleaser

inline LockReleaser::~LockReleaser()
{
    if (m_pLock)
        m_pLock->unlock();
}

inline auto LockReleaser::operator=(LockReleaser&& other) noexcept -> LockReleaser&
{
    if (this->m_pLock)
        this->m_pLock->unlock();
    this->m_pLock = other.m_pLock;
    other.m_pLock = nullptr;
    return *this;
}

inline void LockReleaser::unlock()
{
    *this = LockReleaser();
}

}} // Module namespace
