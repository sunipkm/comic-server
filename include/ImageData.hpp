#ifndef __IMAGEDATA_H__
#define __IMAGEDATA_H__
#include <stdlib.h>

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

    int m_imageWidth;
    int m_imageHeight;

    unsigned short *m_imageData;

    unsigned char *m_jpegData;
    int sz_jpegData;

    bool convert_jpeg;

public:
    float exposure;
    int bin_x, bin_y;

public:
    CImageData();
    CImageData(int imageWidth, int imageHeight, unsigned short *imageData = NULL, bool enableJpeg = false);

    CImageData(const CImageData &rhs);
    CImageData &operator=(const CImageData &rhs);

    ~CImageData();

    void ClearImage();

    bool HasData() const { return m_imageData != NULL; }

    int GetImageWidth() const { return m_imageWidth; }
    int GetImageHeight() const { return m_imageHeight; }

    void GetJPEGData(unsigned char *&ptr, int &sz);

    ImageStats GetStats() const;

    const unsigned short *const GetImageData() const { return m_imageData; }
    unsigned short *const GetImageData() { return m_imageData; }

    void Add(const CImageData &rsh);
    void ApplyBinning(int x, int y);
    void FlipHorizontal();

private:
    void ConvertJPEG();
};

#endif // __IMAGEDATA_H__
