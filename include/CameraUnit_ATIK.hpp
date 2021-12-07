#ifndef __CAMERAUNIT_ATIK_H__
#define __CAMERAUNIT_ATIK_H__

#include "CameraUnit.hpp"
#include "CriticalSection.hpp"
#include "AtikCameras.h"

class CCameraUnit_ATIK : public CCameraUnit
{
    ArtemisHandle hCam;
    struct ARTEMISPROPERTIES props;

    bool m_initializationOK;
    CriticalSection criticalSection_;
    bool cancelCapture_;
    std::string status_;
    CriticalSection statusCriticalSection_;

    bool hasshutter;

    int numtempsensors;

    float exposure_;
    bool exposure_updated_;

    bool requestShutterOpen_;
    bool shutter_updated_;

    int binningX_;
    int binningY_;

    int imageLeft_;
    int imageRight_;
    int imageTop_;
    int imageBottom_;

    int roiLeft;
    int roiRight;
    int roiTop;
    int roiBottom;

    bool roi_updated_;

    int CCDWidth_;
    int CCDHeight_;

    char cam_name[100];

public:
    CCameraUnit_ATIK();
    ~CCameraUnit_ATIK();

    // Control
    CImageData CaptureImage(long int &retryCount);
    void CancelCapture();

    // Accessors
    bool CameraReady() const { return m_initializationOK; }
    const char *CameraName() const { return cam_name; }

    void SetExposure(float exposureInSeconds);
    float GetExposure() const { return exposure_; }

    void SetShutterIsOpen(bool open);

    void SetReadout(int ReadSpeed);

    void SetTemperature(double temperatureInCelcius);
    double GetTemperature() const;

    void SetBinningAndROI(int x, int y, int x_min = 0, int x_max = 0, int y_min = 0, int y_max = 0);
    int GetBinningX() const { return binningX_; }
    int GetBinningY() const { return binningY_; }
    const ROI *GetROI() const;

    std::string GetStatus() const { return status_; }

    int GetCCDWidth() const { return CCDWidth_; }
    int GetCCDHeight() const { return CCDHeight_; }

private:
    bool StatusIsIdle();
    void SetShutter(bool open);
    bool HasError(int error, unsigned int line) const;
    int ArtemisGetCameraState(ArtemisHandle h);
};

#endif // __CAMERAUNIT_PI_H__
