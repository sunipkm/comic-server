/**
 * @file ImageData.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Common Image Data Storage Format
 * @version 0.1
 * @date 2022-01-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __IMAGEDATA_HPP__
#define __IMAGEDATA_HPP__
#include <stdlib.h>
#include <stdint.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#define OS_Windows
#endif
#include <string>

/**
 * @brief Image Data Statistics Storage Class
 * 
 */
class ImageStats
{
    int min_;
    int max_;
    double mean_;
    double stddev_;

public:
    /**
     * @brief Construct a new Image Stats object
     * 
     * @param min Minimum pixel count
     * @param max Maximum pixel count
     * @param mean Mean pixel count
     * @param stddev Standard deviation of pixel count
     */
    ImageStats(int min, int max, double mean, double stddev)
        : min_(min), max_(max), mean_(mean), stddev_(stddev) {}
    /**
     * @brief Get the minimum pixel count
     * 
     * @return int 
     */
    int GetMinValue() const { return min_; }
    /**
     * @brief Get the maximum pixel count
     * 
     * @return int 
     */
    int GetMaxValue() const { return max_; }
    /**
     * @brief Get the mean pixel count
     * 
     * @return int 
     */
    double GetMeanValue() const { return mean_; }
    /**
     * @brief Get the pixel count standard deviation
     * 
     * @return int 
     */
    double GetStandardDeviationValue() const { return stddev_; }
};

/**
 * @brief Class to contain 16-bit raw image data
 * 
 */
class CImageData
{
    int m_imageHeight;
    int m_imageWidth;

    float m_exposureTime;
    int m_binX;
    int m_binY;
    float m_temperature;
    uint64_t m_timestamp;

    std::string m_cameraName;

    unsigned short *m_imageData;

    unsigned char *m_jpegData;
    int sz_jpegData;

    bool convert_jpeg;

    int JpegQuality;
    int pixelMin;
    int pixelMax;
    bool autoscale;

public:
    float exposure;
    int bin_x, bin_y;

public:
    /**
     * @brief Construct a new CImageData object
     * 
     */
    CImageData();
    /**
     * @brief Construct a new CImageData object from image data
     * 
     * @param imageWidth Width of image
     * @param imageHeight Height of image
     * @param imageData [optional] Pointer to image data
     * @param exposureTime [optional] Exposure length of image
     * @param enableJpeg [optional] Enable JPEG conversion
     * @param JpegQuality [optional] Quality of JPEG conversion
     * @param pixelMin [optional] JPEG image scaling pixel count minimum, -1 for default (0x0000), overriden by autoscale flag
     * @param pixelMax [optional] JPEG image scaling pixel count maximum, -1 for default (0xffff), overriden by autoscale flag
     * @param autoscale [optional] Auto-scale JPEG image brightness based on data
     */
    CImageData(int imageWidth, int imageHeight, unsigned short *imageData = NULL, float exposureTime = 0, int binX = 1, int binY = 1, float temperature = 0, uint64_t timestamp = 0, std::string cameraName = "", bool enableJpeg = false, int JpegQuality = 100, int pixelMin = -1, int pixelMax = -1, bool autoscale = true);

    /**
     * @brief Construct a new CImageData object from another CImageData object
     * 
     * @param rhs CImageData object
     */
    CImageData(const CImageData &rhs);
    /**
     * @brief Check if two CImageData objects are equal
     * 
     * @param rhs 
     * @return CImageData& 
     */
    CImageData &operator=(const CImageData &rhs);

    ~CImageData();

    /**
     * @brief Clear existing data
     * 
     */
    void ClearImage();

    /**
     * @brief Returns if the container contains image data
     * 
     * @return true 
     * @return false 
     */
    bool HasData() const { return m_imageData != NULL; }
    /**
     * @brief Get the image width
     * 
     * @return int 
     */
    int GetImageWidth() const { return m_imageWidth; }
    /**
     * @brief Get the image height
     * 
     * @return int 
     */
    int GetImageHeight() const { return m_imageHeight; }
    /**
     * @brief Get the image exposure in seconds
     * 
     * @return int 
     */
    float GetImageExposure() const { return m_exposureTime; }
    /**
     * @brief Set the image exposure (should be used by CCameraUnit only)
     * 
     * @param exposure Exposure in seconds
     */
    void SetImageExposure(float exposure) { m_exposureTime = exposure; }
    /**
     * @brief Set metadata for the image
     * 
     * @param exposureTime Exposure for the image
     * @param binX X axia bin
     * @param binY Y axis bin
     * @param temperature CCD Temperature
     * @param timestamp Image timestamp
     * @param cameraName Camera name
     */
    void SetImageMetadata(float exposureTime, int binX = 1, int binY = 1, float temperature = 0, uint64_t timestamp = 0, std::string cameraName = "");
    /**
     * @brief Retrieve JPEG image corresponding to raw data
     * 
     * @param ptr Pointer to JPEG image data
     * @param sz Size of JPEG image data
     */
    void GetJPEGData(unsigned char *&ptr, int &sz);
    /**
     * @brief Set quality of JPEG image
     * 
     * @param quality Quality in % (10 - 100)
     */
    void SetJPEGQuality(int quality = 100)
    {
        JpegQuality = quality;
        JpegQuality = JpegQuality < 0 ? 10 : JpegQuality;
        JpegQuality = JpegQuality > 100 ? 100 : JpegQuality;
    }
    /**
     * @brief Set pixel scaling values for JPEG image conversion
     * 
     * @param min Minimum pixel count, this is the minimum brightness [dark level]
     * @param max Maximum pixel count, this is the maximum brightness [bright level]
     */
    void SetJPEGScaling(int min = -1, int max = -1)
    {
        pixelMin = min;
        pixelMax = max;
    }
    /**
     * @brief Enable/disable automatic scaling of image brightness based on pixel data
     * 
     * @param autoscale 
     */
    void SetJPEGScaling(bool autoscale);
    /**
     * @brief Get statistics on image data
     * 
     * @return ImageStats Statistics data container
     */
    ImageStats GetStats() const;
    /**
     * @brief Get the pointer to image data
     * 
     * @return const unsigned short* const 
     */
    const unsigned short *const GetImageData() const { return m_imageData; }
    /**
     * @brief Get the pointer to image data
     * 
     * @return unsigned short* const 
     */
    unsigned short *const GetImageData() { return m_imageData; }
    /**
     * @brief Stack data from another image
     * 
     * @param rsh Image container
     */
    void Add(const CImageData &rsh);
    /**
     * @brief Software-binning of image data
     * 
     * @param x X axis binning
     * @param y Y axis binning
     */
    void ApplyBinning(int x, int y);
    /**
     * @brief Flip image horizontally
     * 
     */
    void FlipHorizontal();
    /**
     * @brief Find optimum exposure from this exposure
     * 
     * @param targetExposure Target exposure time (output)
     * @param bin Target bin size (output)
     * @param percentilePixel Pixel percentile target (input, default: 80 percentile)
     * @param pixelTarget Value terget for pixel percentile (input, default: 40000)
     * @param maxAllowedExposure Maximum allowed exposure time (input, default: 10 s)
     * @param maxAllowedBin Maximum allowed binning (input, default: 4)
     * @param numPixelExclusion Number of pixels to be excluded from calculation (input, default: 100)
     * @param pixelTargetUncertainty Value target uncertainty (inpit, default: 5000)
     * @return bool Returns true.
     */
    bool FindOptimumExposure(float &targetExposure, int &bin, float percentilePixel = 0.8, int pixelTarget = 40000, float maxAllowedExposure = 10.0, int maxAllowedBin = 4, int numPixelExclusion = 100, int pixelTargetUncertainty = 5000);
    /**
     * @brief Find optimum exposure from this exposure without binning adjustment
     * 
     * @param targetExposure Target exposure time (output)
     * @param percentilePixel Pixel percentile target (input, default: 80 percentile)
     * @param pixelTarget Value terget for pixel percentile (input, default: 40000)
     * @param maxAllowedExposure aximum allowed exposure time (input, default: 10 s)
     * @param numPixelExclusion Number of pixels to be excluded from calculation (input, default: 100) 
     * @param pixelTargetUncertainty Value target uncertainty (inpit, default: 5000)
     * @return bool Returns true.
     */
    bool FindOptimumExposure(float &targetExposure, float percentilePixel = 0.8, int pixelTarget = 40000, float maxAllowedExposure = 10.0, int numPixelExclusion = 100, int pixelTargetUncertainty = 5000);
    /**
     * @brief Save image contained in CImageData
     * 
     * @param filePrefix File name prefix
     * @param DirPrefix Directory name
     * @param i Image index
     * @param n Out of n
     * @param outString Status output string pointer
     * @param outStringSz Status output string max size
     */
    void SaveFits(char *filePrefix, char *DirPrefix, int i = -1, int n = -1, char *outString = NULL, ssize_t outStringSz = 0);

private:
    /**
     * @brief Convert raw image to JPEG image with preset settings
     * 
     */
    void ConvertJPEG();
    /**
     * @brief Return minimum pixel count
     * 
     * @return uint16_t 
     */
    uint16_t DataMin();
    /**
     * @brief Return maximum pixel count
     * 
     * @return uint16_t 
     */
    uint16_t DataMax();
};

#endif // __IMAGEDATA_HPP__
