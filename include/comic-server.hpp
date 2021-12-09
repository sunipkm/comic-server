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

#ifndef _COMIC_SERVER_HPP_
#define _COMIC_SERVER_HPP_

#include "CameraUnit_ATIK.hpp"
#include "network/network_common.hpp"
#include "network_server.hpp"
#include "meb_print.h"
#include "ProtQueue.hpp"

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
    int32_t type;
    int32_t size;
} netimg_meta;

typedef enum
{
    JPEGMONO = 15,
    JPEGRGBA = 20,
    JPEGRGB = 25,
    RAW8 = 30,
    RAW16 = 35
} netimg_type;

typedef enum
{
    CMD = 20, // command from controller
    DATA,     // data sent to controller
    TELEM     // telemetry sent to controller
} comic_netdata;

static CCameraUnit *cam = nullptr;           // Camera object
static CImageData image;                     // Image object
static int32_t CCDTemperature;               // CCD Temperature, in 100th of degree
static int32_t CCDTemperatureTarget = -3000; // CCD Temperature Target, in 100th of degree
static uint64_t ExposureCadenceMs = 1000;    // Time between exposures in ms
static ProtQueue<NetFrame *> tx_queue;       // Transmit queue
static NetVertex CtrlVertex;                 // Controller netvertex

void *CameraThread(void *_inout);
void *ServerThread(void *_inout);
void *CoolerThread(void *_inout);

#include <chrono>

static inline uint64_t getTime()
{
    return ((std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())).time_since_epoch())).count());
}

#endif // _COMIC_SERVER_HPP_