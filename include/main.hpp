/**
 * @file main.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-12-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef __MAIN_HPP__
#define __MAIN_HPP__

#include <stdint.h>

typedef enum
{
    CMD = 20, // command from controller
    DATA,     // data sent to controller
    TELEM     // telemetry sent to controller
} comic_netdata;

#include <chrono>

static inline uint64_t getTime()
{
    return ((std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())).time_since_epoch())).count());
}

#endif // __MAIN_HPP__