/**
 * @file RingBuf.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Ring buffer implementation
 * @version 0.1
 * @date 2021-12-09
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _RING_BUF_HPP_
#define _RING_BUF_HPP_

#include <stdexcept>

template <class T>
/**
 * @brief Ring buffer class for any data of numeric type.
 * 
 */
class RingBuf
{
public:
    /**
     * @brief Construct a new empty ring buffer.
     * 
     */
    RingBuf() : data(nullptr), idx(0), full(false), size(0)
    {
    }

    /**
     * @brief Construct a new ring buffer.
     * 
     * @param size Size of ring buffer.
     */
    RingBuf(int size) : data(nullptr), idx(0), full(false)
    {
        this->size = size;
        Initialize(size);
    }

    /**
     * @brief Destroy the ring buffer.
     * 
     */
    ~RingBuf()
    {
        if (HasData())
            delete[] data;
    }

    /**
     * @brief Initialize an empty ring buffer.
     * 
     * @param size Size of ring buffer.
     */
    void Initialize(int size)
    {
        if (size < 1)
        {
            std::invalid_argument("Buffer size can not be negative or zero.");
        }
        this->size = size;
        if (data != nullptr)
            delete[] data;
        data = nullptr;
        data = new T[size];
        for (int i = 0; i < size; i++)
            data[i] = 0;
    }

    /**
     * @brief Check if ring buffer has been initialized.
     * 
     * @return bool 
     */
    bool HasData() const { return size > 0; }

    /**
     * @brief Check if ring buffer is full.
     * Ring buffer is considered full if number of elements 
     * pushed in is at least equal to the size of the ring buffer.
     * 
     * @return bool
     */
    bool IsFull() const { return full; }

    /**
     * @brief Get the index at data was pushed in last.
     * 
     * @return int Current index of ring buffer.
     */
    int GetIndex() const { return idx; }

    /**
     * @brief Get data at index i relative to the current index.
     * i = 0 returns the element at current index.
     * i = 1 returns the element at the previous index etc.
     * 
     * @param i Ring buffer index
     * @return T& Reference to the element at index idx - i.
     */
    T &operator[](int i)
    {
        if (!HasData())
        {
            throw std::invalid_argument("Buffer not initialized.");
        }
        if (i < 0)
        {
            throw std::invalid_argument("Index can not be negative.");
        }
        if (i >= size)
        {
            i = i % size;
        }
        int index = size - i + idx;
        return data[index];
    }
    
    /**
     * @brief Get data at the absolute index i
     * 
     * @param i Absolute array index
     * @return T& Reference to the element at the absolute index i
     */
    T& at(int i)
    {
        if (!HasData())
        {
            throw std::invalid_argument("Buffer not initialized.");
        }
        if (i < 0)
        {
            throw std::invalid_argument("Index can not be negative.");
        }
        if (i >= size)
        {
            throw std::invalid_argument("Index > size, invalid.")
        }
        return data[i];
    }

    /**
     * @brief Push data into the ring buffer.
     * This process iterates the index.
     * 
     * @param data Data to be pushed into the buffer.
     */
    void push(const T &data)
    {
        if (!HasData())
        {
            throw std::invalid_argument("Buffer not initialized.");
        }
        if (idx == size - 1)
            full = true;
        idx = (idx + 1) % size;
        (this->data)[idx] = data;
    }

    /**
     * @brief Clear data in the ring buffer.
     * 
     */
    void clear()
    {
        if (!HasData())
        {
            throw std::invalid_argument("Buffer not initialized.");
        }
        idx = 0;
        full = false;
        for (int i = 0; i < size; i++)
            data[i] = 0;
    }

    /**
     * @brief Get the size of the ring buffer
     * 
     * @return const int 
     */
    const int GetSize() const { return size; }

private:
    T *data;
    int idx;
    bool full;
    int size;
};

#endif // _RING_BUF_HPP_