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

#include <mutex>
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
    std::mutex cs_;

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
        std::lock_guard<std::mutex> lock(cs_);
        return q_.empty();
        
    }
    size_t size()
    {
        std::lock_guard<std::mutex> lock(cs_);
        return q_.size();
    }
    const T &front() const
    {
        std::lock_guard<std::mutex> lock(cs_);
        return q_.front();
    }
    const T &back() const
    {
        std::lock_guard<std::mutex> lock(cs_);
        return q_.back();
    }
    void push(const T &val)
    {
        std::lock_guard<std::mutex> lock(cs_);
        q_.push(val);
    }
    void push(T &&val)
    {
        std::lock_guard<std::mutex> lock(cs_);
        q_.push(val);
    }
    template <typename... _Args>
#if __cplusplus > 201402L
    decltype(auto) void emplace(_Args &&...__args)
#else
    void emplace(_Args &&...__args)
#endif
    {
        std::lock_guard<std::mutex> lock(cs_);
        q_.emplace(std::forward<_Args>(__args)...);
    }
    void pop()
    {
        std::lock_guard<std::mutex> lock(cs_);
        q_.pop();
    }
    void swap(ProtQueue &x) noexcept
    {
        std::unique_lock<std::mutex> l1(cs_, std::defer_lock);
        std::unique_lock<std::mutex> l2(x.cs_, std::defer_lock);
        std::lock(l1, l2);
        this->swap(x);
    }
};
#endif // _PROT_QUEUE_HPP_