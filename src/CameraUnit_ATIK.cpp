// CameraUnit.cpp : implementation file
//
#include "CameraUnit_ATIK.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#if !defined(OS_Windows)
#include <unistd.h>
static inline void Sleep(int dwMilliseconds)
{
    usleep(dwMilliseconds * 1000);
}
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define eprintf(str, ...)                                                   \
    {                                                                       \
        fprintf(stderr, "%s, %d: " str, __func__, __LINE__, ##__VA_ARGS__); \
        fflush(stderr);                                                     \
    }

#define eprintlf(str, ...) eprintf(str "\n", ##__VA_ARGS__)

bool CCameraUnit_ATIK::HasError(int error, unsigned int line) const
{
    switch (error)
    {
    default:
        printf("%s, %d: ATIK Error %d\n", __FILE__, line, error);
        fflush(stdout);
        return true;
    case ARTEMIS_OK:
        return false;

#define ARTEMIS_ERROR(x)                                           \
    case x:                                                        \
        printf("%s, %d: ARTEMIS error: " #x "\n", __FILE__, line); \
        fflush(stdout);                                            \
        return true;

        ARTEMIS_ERROR(ARTEMIS_INVALID_PARAMETER)
        ARTEMIS_ERROR(ARTEMIS_NOT_CONNECTED)
        ARTEMIS_ERROR(ARTEMIS_NOT_IMPLEMENTED)
        ARTEMIS_ERROR(ARTEMIS_NO_RESPONSE)
        ARTEMIS_ERROR(ARTEMIS_NOT_INITIALIZED)
        ARTEMIS_ERROR(ARTEMIS_INVALID_FUNCTION)
        ARTEMIS_ERROR(ARTEMIS_OPERATION_FAILED)
#undef ARTEMIS_ERROR
    }
}

int CCameraUnit_ATIK::ArtemisGetCameraState(ArtemisHandle h)
{
    int state = ArtemisCameraState(h);
    switch (state)
    {
#define ARTEMIS_STATE(x)           \
    case x:                        \
        status_ = std::string(#x); \
        break;

        ARTEMIS_STATE(CAMERA_ERROR)
        ARTEMIS_STATE(CAMERA_IDLE)
        ARTEMIS_STATE(CAMERA_WAITING)
        ARTEMIS_STATE(CAMERA_EXPOSING)
        ARTEMIS_STATE(CAMERA_READING)
        ARTEMIS_STATE(CAMERA_DOWNLOADING)
        ARTEMIS_STATE(CAMERA_FLUSHING)
#undef ARTEMIS_STATE
    }
}

CCameraUnit_ATIK::CCameraUnit_ATIK()
    : binningX_(1), binningY_(1), cancelCapture_(true), CCDHeight_(0), CCDWidth_(0), imageLeft_(0), imageRight_(0), imageTop_(0), imageBottom_(0), roiLeft(0), roiRight(0), roiBottom(0), roiTop(0), exposure_(0), lastError_(0), m_initializationOK(false), requestShutterOpen_(true)
{
    // do initialization stuff
    short numcameras = 0;

    // initialize camera

#ifdef _WIN32
    // First: Try to load the DLL:
    if (!ArtemisLoadDLL("AtikCameras.dll"))
    {

        return;
    }
#endif

    // Now Check API / DLL versions
    int apiVersion = ArtemisAPIVersion();
    int dllVersion = ArtemisDLLVersion();
    if (apiVersion != dllVersion)
    {
        eprintlf("Version do not match! API: %d DLL: %d", apiVersion, dllVersion);
        return;
    }

    // get number of cameras and names
    numcameras = ArtemisDeviceCount();
    if (numcameras == 0)
    {
        return;
    }
    if (ArtemisDeviceInUse(0))
    {
        eprintlf("Device 0 already in use");
        return;
    }
    if (!ArtemisDeviceIsCamera(0))
    {
        eprintf("Device 0 is not a camera");
        return;
    }
    if (!ArtemisDeviceName(0, cam_name))
    {
        eprintlf("Could not get camera name");
        return;
    }
    // Open the camera
    hCam = ArtemisConnect(0);
    if (!ArtemisIsConnected(hCam))
    {
        eprintlf("Could not connect to ATIK handle %p, returning", hCam);
        return;
    }
    // Get camera properties
    if (HasError(ArtemisProperties(hCam, &props), __LINE__))
    {
        eprintlf("Could not get camera properties");
        goto close;
    }

    CCDHeight_ = int(props.nPixelsY);
    CCDWidth_ = int(props.nPixelsX);

    // Set preview mode to false
    HasError(ArtemisSetPreview(hCam, false), __LINE__);

    // Set binning to 1x1
    HasError(ArtemisBin(hCam, 1, 1), __LINE__);

    // Set 8-bit mode to false
    HasError(ArtemisEightBitMode(hCam, false), __LINE__);

    // Get number of temperature sensors
    HasError(ArtemisTemperatureSensorInfo(hCam, 0, &numtempsensors), __LINE__);

    // Get shutter caps
    HasError(ArtemisCanControlShutter(hCam, &hasshutter), __LINE__);

    // Initialization done
    m_initializationOK = true;

    return;
close:
    ArtemisDisconnect(hCam);
    m_initializationOK = false;
}

CCameraUnit_ATIK::~CCameraUnit_ATIK()
{
    CriticalSection::Lock lock(criticalSection_);
    while (!ArtemisDisconnect(hCam));
    m_initializationOK = false;
#ifdef _WIN32
    ArtemisUnLoadDLL();
#endif
}

void CCameraUnit_ATIK::CancelCapture()
{
    CriticalSection::Lock lock(criticalSection_);
    cancelCapture_ = true;
    // abort acquisition
    ArtemisAbortExposure(hCam);
}

// ----------------------------------------------------------

CImageData CCameraUnit_ATIK::CaptureImage(long int &retryCount)
{
    printf("Starting capture image\n");
    CriticalSection::Lock lock(criticalSection_);
    CImageData retVal;
    cancelCapture_ = false;

    void *pImgBuf;

    int x, y, w, h, binx, biny;

    int sleep_time_ms = 0;

    int cameraState = 0;

    int exposure_ms = exposure_ * 1000;
    if (exposure_ms < 1)
        exposure_ms = 1;

    printf("Exposure: %d ms\n", exposure_ms);
    if (!m_initializationOK)
    {
        goto exit_err;
    }

    if (HasError(ArtemisStartExposureMS(hCam, exposure_ms), __LINE__))
    {
        goto exit_err;
    }
    printf("Exposure started\n");
    sleep_time_ms = 1000 * ArtemisExposureTimeRemaining(hCam); // sleep time in ms
    printf("Sleep time: %d ms\n", sleep_time_ms);
    if (sleep_time_ms < 0)
        sleep_time_ms = 0;
    lock.Unlock();

    Sleep(sleep_time_ms);

    lock.Relock();
    printf("Out of sleep\n");
    while (!ArtemisImageReady(hCam))
    {
        cameraState = ArtemisGetCameraState(hCam);
        if (cameraState == CAMERA_DOWNLOADING)
        {
            status_ += std::string(" Download: ");
            status_ += std::to_string(ArtemisDownloadPercent(hCam));
            status_ += " %";
        }
        printf("Status: %s", status_.c_str());
        Sleep(5);
    }

    if (HasError(ArtemisGetImageData(hCam, &x, &y, &w, &h, &binx, &biny), __LINE__))
    {
        eprintlf("Error getting image data");
        goto exit_err;
    }
    retVal = CImageData(w, h);
    binningX_ = binx;
    binningY_ = biny;

    pImgBuf = ArtemisImageBuffer(hCam);
    if (pImgBuf == NULL)
    {
        eprintlf("Image buffer is NULL");
        goto exit_err;
    }
    memcpy(retVal.GetImageData(), pImgBuf, w * h * 2);
exit_err:
    // printf("Exiting capture\n");
    return retVal;
}

void CCameraUnit_ATIK::SetTemperature(double temperatureInCelcius)
{
    if (!m_initializationOK)
    {
        return;
    }

    int16_t settemp;
    settemp = temperatureInCelcius * 100;

    HasError(ArtemisSetCooling(hCam, settemp), __LINE__);
}

// get temperature is done
double CCameraUnit_ATIK::GetTemperature() const
{
    if (!m_initializationOK)
    {
        return INVALID_TEMPERATURE;
    }

    int temperature = 0;
    for (int i = 0; i < numtempsensors; i++)
        HasError(ArtemisTemperatureSensorInfo(hCam, i + 1, &temperature), __LINE__);

    double retVal;

    retVal = double(temperature) / 10;
    return retVal;
}

//
void CCameraUnit_ATIK::SetBinningAndROI(int binX, int binY, int x_min, int x_max, int y_min, int y_max)
{
    if (!m_initializationOK)
    {
        return;
    }

    CriticalSection::Lock lock(criticalSection_);
    if (!m_initializationOK)
    {
        return;
    }

    if (binX < 1)
        binX = 1;
    if (binX > 16)
        binX = 16;

    bool change_bin = false;
    if (binningX_ != binX)
    {
        change_bin = true;
    }

    if (binY < 1)
        binY = 1;
    if (binY > 16)
        binY = 16;

    if (binningY_ != binY)
    {
        change_bin = true;
    }

    if (change_bin)
    {
        if (HasError(ArtemisBin(hCam, binX, binY), __LINE__))
            return;
        binningY_ = binY;
        binningX_ = binX;
    }

    imageLeft_ = x_min / binningX_;
    imageRight_ = (x_max - x_min) / binningX_ + imageLeft_;
    imageTop_ = y_min / binningY_;
    imageBottom_ = (y_max - y_min) / binningY_ + imageTop_;

    if (imageRight_ > GetCCDWidth() - 1)
        imageRight_ = GetCCDWidth() - 1;
    if (imageLeft_ < 0)
        imageLeft_ = 0;
    if (imageRight_ <= imageLeft_)
        imageRight_ = GetCCDWidth() - 1;

    if (imageTop_ > GetCCDHeight() - 1)
        imageTop_ = GetCCDHeight() - 1;
    if (imageBottom_ < 0)
        imageBottom_ = 0;
    if (imageTop_ <= imageBottom_)
        imageTop_ = GetCCDHeight() - 1;

    roiLeft = imageLeft_;
    roiRight = imageRight_ + 1;
    roiBottom = imageBottom_;
    roiTop = imageTop_ + 1;

    HasError(ArtemisSubframe(hCam, imageLeft_, imageTop_, imageBottom_ - imageTop_, imageRight_ - imageLeft_), __LINE__);

    // printf("%d %d, %d %d | %d %d\n", binningX_, binningY_, imageLeft_, imageRight_, imageBottom_, imageTop_);
}

const ROI *CCameraUnit_ATIK::GetROI() const
{
    static ROI roi;
    roi.x_min = roiLeft;
    roi.x_max = roiRight;
    roi.y_min = roiBottom;
    roi.y_max = roiTop;
    return &roi;
}

void CCameraUnit_ATIK::SetShutter(bool open)
{
    if (!m_initializationOK)
    {
        return;
    }
    if (hasshutter)
    {
        if (open)
        {
            HasError(ArtemisOpenShutter(hCam), __LINE__);
            requestShutterOpen_ = true;
        }
        else
        {
            HasError(ArtemisCloseShutter(hCam), __LINE__);
            requestShutterOpen_ = false;
        }
    }
}

void CCameraUnit_ATIK::SetShutterIsOpen(bool open)
{
    SetShutter(open);
}

// exposure time done
void CCameraUnit_ATIK::SetExposure(float exposureInSeconds)
{
    if (!m_initializationOK)
    {
        return;
    }

    if (exposureInSeconds <= 0)
    {
        exposureInSeconds = 0.0;
    }

    long int maxexposurems = exposureInSeconds * 1000;

    if (maxexposurems > 10 * 60 * 1000) // max exposure 10 minutes
        maxexposurems = 10 * 60 * 1000;
    CriticalSection::Lock lock(criticalSection_);
    exposure_ = maxexposurems * 0.001; // 1 ms increments only
}

void CCameraUnit_ATIK::SetReadout(int ReadSpeed)
{
    if (!m_initializationOK)
    {
        return;
    }

    return;
}