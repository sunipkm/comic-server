#include "CriticalSection.hpp"

#if defined(OS_Windows)
CriticalSection::CriticalSection()
{
    ::InitializeCriticalSection(&critSect_);
}

CriticalSection::~CriticalSection()
{
    ::DeleteCriticalSection(&critSect_);
}

void CriticalSection::LockIt() const
{
    ::EnterCriticalSection(&critSect_);
}

void CriticalSection::UnlockIt() const
{
    ::LeaveCriticalSection(&critSect_);
}
#else
CriticalSection::CriticalSection()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&critSect_, &attr);
    pthread_mutexattr_destroy(&attr);
}

CriticalSection::~CriticalSection()
{
    pthread_mutex_destroy(&critSect_);
}

void CriticalSection::LockIt() const
{
    pthread_mutex_lock(&critSect_);
}

void CriticalSection::UnlockIt() const
{
    pthread_mutex_unlock(&critSect_);
}
#endif
CriticalSection::Lock::Lock(const CriticalSection &cs)
    : cs_(cs), locked_(false)
{
    cs_.LockIt();
    locked_ = true;
}

CriticalSection::Lock::Lock(const Lock &rhs) // copy constructor
    : cs_(rhs.cs_), locked_(false)
{
    if (rhs.locked_)
    {
        Relock();
    }
}

void CriticalSection::Lock::Unlock()
{
    if (locked_)
    {
        cs_.UnlockIt();
        locked_ = false;
    }
}

void CriticalSection::Lock::Relock()
{
    cs_.LockIt();
    locked_ = true;
}

CriticalSection::Lock::~Lock()
{
    if (locked_)
    {
        cs_.UnlockIt();
        locked_ = false;
    }
}
