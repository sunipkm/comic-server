/**
 * @file ProtQueue.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Protected Queue Implementation for thread-safe operations. Depends on CriticalSection code for cross-platform purposes.
 * @version 0.1
 * @date 2021-12-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _PROT_QUEUE_HPP_
#define _PROT_QUEUE_HPP_

#include "CriticalSection.hpp"
#include <queue>

/**
 * @brief Wrapper around std::queue to ensure thread safety. Standard std::queue functions apply.
 * 
 * @tparam T Template type
 */
template <class T>
class ProtQueue
{
private:
    std::queue<T> q_;
    CriticalSection cs_;

public:
    ProtQueue()
    {
    }
    ProtQueue(const ProtQueue &other)
    {
        q_ = other.q_;
        cs_ = other.cs_;
    }
    ProtQueue(ProtQueue &&other)
    {
        q_ = other->q_;
        cs_ = other->cs_;
    }
    bool empty()
    {
        CriticalSection::Lock lock(cs_);
        return q_.empty();
    }
    size_t size()
    {
        CriticalSection::Lock lock(cs_);
        return q_.size();
    }
    const T &front() const
    {
        CriticalSection::Lock lock(cs_);
        return q_.front();
    }
    const T &back() const
    {
        CriticalSection::Lock lock(cs_);
        return q_.back();
    }
    void push(const T &val)
    {
        CriticalSection::Lock lock(cs_);
        q_.push(val);
    }
    void push(T &&val)
    {
        CriticalSection::Lock lock(cs_);
        q_.push(val);
    }
    template <typename... _Args>
#if __cplusplus > 201402L
    decltype(auto) void emplace(_Args &&...__args)
#else
    void emplace(_Args &&...__args)
#endif
    {
        CriticalSection::Lock lock(cs_);
        q_.emplace(std::forward<_Args>(__args)...);
    }
    void pop()
    {
        CriticalSection::Lock lock(cs_);
        q_.pop();
    }
    void swap(ProtQueue &x) noexcept
    {
        CriticalSection::Lock lock(cs_);
        CriticalSection::Lock lock2(x.cs_);
        this->swap(x);
    }
};
#endif // _PROT_QUEUE_HPP_