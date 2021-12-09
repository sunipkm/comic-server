/**
 * @file CriticalSection.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-12-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef _CRITICALSECTION_HPP_
#define _CRITICALSECTION_HPP_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#define OS_Windows
#else
#include <pthread.h>
#define CRITICAL_SECTION pthread_mutex_t
#endif
/**
 * @brief CriticalSection is a cross-platform mutex class that supports
 * auto unlocking after the Lock object on the mutex goes out of context.
 * Supports multiple locks from the same thread.
 * 
 */
class CriticalSection
{
    mutable CRITICAL_SECTION critSect_;

    void LockCS() const;
    void UnlockCS() const;

public:
    /**
     * @brief Construct a new Critical Section object
     * 
     */
    CriticalSection();
    ~CriticalSection();

    /**
     * @brief Manages lock on a CriticalSection (mutex)
     * 
     */
    class Lock
    {
        const CriticalSection &cs_;
        bool locked_;

    public:
        /**
         * @brief Construct a new Lock object and acquire lock on a CriticalSection
         * 
         * @param cs CriticalSection mutex
         */
        Lock(const CriticalSection &cs);
        /**
         * @brief Copy an existing lock object
         * 
         */
        Lock(const Lock &);
        /**
         * @brief Destroy the Lock object, automatically releases the lock on the CriticalSection
         * 
         */
        ~Lock();
        /**
         * @brief Unlock a locked CriticalSection mutex
         * 
         */
        void Unlock(void);
        /**
         * @brief Reacquire lock on a CriticalSection mutex
         * 
         */
        void Relock(void);

    private:
        // disallow assignment
        Lock &operator=(const Lock &);
    };
    friend class CriticalSection::Lock;
};

#endif // _CRITICALSECTION_HPP_