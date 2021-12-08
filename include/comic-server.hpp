/**
 * @file comic-server.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-12-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "CameraUnit_ATIK.hpp"

/**
 * @brief Network Image Metadata
 * 
 * @return typedef struct 
 */
typedef struct __attribute__((packed))
{
    uint16_t width;
    uint16_t height;
    int32_t temperature;
    uint32_t exposure_ms;
    uint64_t tstamp;
    int32_t size;
} netimg_meta;

static CCameraUnit *cam = nullptr; // Camera object

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
