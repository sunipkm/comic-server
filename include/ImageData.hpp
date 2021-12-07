#ifndef __IMAGEDATA_H__
#define __IMAGEDATA_H__
#include <stdlib.h>
#include <stdint.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#define OS_Windows
#endif
/////////////////////////////////////////////////////////////////////////////
// CImageData object

class ImageStats
{
    int min_;
    int max_;
    double mean_;
    double stddev_;

public:
    ImageStats(int min, int max, double mean, double stddev)
        : min_(min), max_(max), mean_(mean), stddev_(stddev) {}

    int GetMinValue() const { return min_; }
    int GetMaxValue() const { return max_; }
    double GetMeanValue() const { return mean_; }
    double GetStandardDeviationValue() const { return stddev_; }
};

class CImageData
{
    int m_imageHeight;
    int m_imageWidth;

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
    CImageData();
    CImageData(int imageWidth, int imageHeight, unsigned short *imageData = NULL, bool enableJpeg = false, int JpegQuality = 100, int pixelMin = -1, int pixelMax = -1, bool autoscale = true);

    CImageData(const CImageData &rhs);
    CImageData &operator=(const CImageData &rhs);

    ~CImageData();

    void ClearImage();

    bool HasData() const { return m_imageData != NULL; }

    int GetImageWidth() const { return m_imageWidth; }
    int GetImageHeight() const { return m_imageHeight; }

    void GetJPEGData(unsigned char *&ptr, int &sz);
    void SetJPEGQuality(int quality = 100)
    {
        JpegQuality = quality;
        JpegQuality = JpegQuality < 0 ? 10 : JpegQuality;
        JpegQuality = JpegQuality > 100 ? 100 : JpegQuality;
    }
    void SetJPEGScaling(int min = -1, int max = -1)
    {
        pixelMin = min;
        pixelMax = max;
    }
    void SetJPEGScaling(bool autoscale);

    ImageStats GetStats() const;

    const unsigned short *const GetImageData() const { return m_imageData; }
    unsigned short *const GetImageData() { return m_imageData; }

    void Add(const CImageData &rsh);
    void ApplyBinning(int x, int y);
    void FlipHorizontal();

private:
    void ConvertJPEG();
    uint16_t DataMin();
    uint16_t DataMax();
};

#endif // __IMAGEDATA_H__
