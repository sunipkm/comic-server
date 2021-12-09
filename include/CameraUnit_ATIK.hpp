#ifndef __CAMERAUNIT_ATIK_HPP__
#define __CAMERAUNIT_ATIK_HPP__

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
    /**
     * @brief Construct a new Atik Camera Object. This will connect to the first available Atik camera.
     * 
     */
    CCameraUnit_ATIK();
    /**
     * @brief Close connection to the connected Atik camera.
     * 
     */
    ~CCameraUnit_ATIK();

    /**
     * @brief Capture image from the connected camera
     * 
     * @param retryCount Unused
     * @return CImageData Container with raw image data
     */
    CImageData CaptureImage(long int &retryCount);
    /**
     * @brief Cancel an ongoing capture
     * 
     */
    void CancelCapture();

    /**
     * @brief Check if camera was initialized properly
     * 
     * @return true 
     * @return false 
     */
    bool CameraReady() const { return m_initializationOK; }
    /**
     * @brief Get the name of the connected camera
     * 
     * @return const char* 
     */
    const char *CameraName() const { return cam_name; }

    /**
     * @brief Set the exposure time in seconds
     * 
     * @param exposureInSeconds 
     */
    void SetExposure(float exposureInSeconds);
    /**
     * @brief Get the currently set exposure
     * 
     * @return float 
     */
    float GetExposure() const { return exposure_; }
    /**
     * @brief Open or close the shutter
     * 
     * @param open 
     */
    void SetShutterIsOpen(bool open);
    /**
     * @brief Set the readout speed (unused)
     * 
     * @param ReadSpeed 
     */
    void SetReadout(int ReadSpeed);
    /**
     * @brief Set the cooler target temperature
     * 
     * @param temperatureInCelcius 
     */
    void SetTemperature(double temperatureInCelcius);
    /**
     * @brief Get the current detector temperature
     * 
     * @return double 
     */
    double GetTemperature() const;
    /**
     * @brief Set the Binning And ROI information
     * 
     * @param x X axis binning
     * @param y Y axis binning
     * @param x_min Leftmost pixel index (unbinned)
     * @param x_max Rightmost pixel index (unbinned)
     * @param y_min Topmost pixel index (unbinned)
     * @param y_max Bottommost pixel index (unbinned)
     */
    void SetBinningAndROI(int x, int y, int x_min = 0, int x_max = 0, int y_min = 0, int y_max = 0);
    /**
     * @brief Get the X binning set on the detector
     * 
     * @return int 
     */
    int GetBinningX() const { return binningX_; }
    /**
     * @brief Get the Y binning set on the detector
     * 
     * @return int 
     */
    int GetBinningY() const { return binningY_; }
    /**
     * @brief Get the currently set region of interest
     * 
     * @return const ROI* 
     */
    const ROI *GetROI() const;
    /**
     * @brief Get the current status string
     * 
     * @return std::string 
     */
    std::string GetStatus() const { return status_; }
    /**
     * @brief Get the detector width in pixels
     * 
     * @return int 
     */
    int GetCCDWidth() const { return CCDWidth_; }
    /**
     * @brief Get the detector height in pixels
     * 
     * @return int 
     */
    int GetCCDHeight() const { return CCDHeight_; }

private:
    bool StatusIsIdle();
    void SetShutter(bool open);
    bool HasError(int error, unsigned int line) const;
    int ArtemisGetCameraState(ArtemisHandle h);
};

#endif // __CAMERAUNIT_ATIK_HPP__
