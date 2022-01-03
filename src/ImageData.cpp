/**
 * @file ImageData.cpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Image Data Storage Methods Implementation
 * @version 0.1
 * @date 2022-01-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "ImageData.hpp"

#include <math.h>
#include <algorithm>
#if !defined(OS_Windows)
#include <string.h>
#else
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif
#include "jpge.hpp"

void CImageData::ClearImage()
{
    if (m_imageData != 0)
        delete[] m_imageData;
    m_imageData = 0;

    m_imageWidth = 0;
    m_imageHeight = 0;

    if (m_jpegData != nullptr)
    {
        delete[] m_jpegData;
        m_jpegData = nullptr;
    }
}

CImageData::CImageData()
    : m_imageHeight(0), m_imageWidth(0), m_exposureTime(0), m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false), JpegQuality(100), pixelMin(-1), pixelMax(-1)
{
    ClearImage();
}

CImageData::CImageData(int imageWidth, int imageHeight, unsigned short *imageData, float exposureTime, bool enableJpeg, int JpegQuality, int pixelMin, int pixelMax, bool autoscale)
    : m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false)
{
    ClearImage();

    if ((imageWidth <= 0) || (imageHeight <= 0))
    {
        return;
    }

    m_imageData = new unsigned short[imageWidth * imageHeight];
    if (m_imageData == NULL)
    {
        return;
    }

    if (imageData)
    {
        memcpy(m_imageData, imageData, imageWidth * imageHeight * sizeof(unsigned short));
    }
    else
    {
        memset(m_imageData, 0, imageWidth * imageHeight * sizeof(unsigned short));
    }
    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;
    m_exposureTime = exposureTime;

    if (enableJpeg)
    {
        convert_jpeg = true;
        this->JpegQuality = JpegQuality;
        this->pixelMin = pixelMin;
        this->pixelMax = pixelMax;
        this->autoscale = autoscale;
        ConvertJPEG();
    }
}

CImageData::CImageData(const CImageData &rhs)
    : m_imageData(NULL), m_jpegData(nullptr), sz_jpegData(-1), convert_jpeg(false)
{
    // printf("RHS called\n");
    ClearImage();

    if ((rhs.m_imageWidth == 0) || (rhs.m_imageHeight == 0) || (rhs.m_imageData == 0))
    {
        return;
    }

    m_imageData = new unsigned short[rhs.m_imageWidth * rhs.m_imageHeight];

    if (m_imageData == 0)
    {
        return;
    }

    memcpy(m_imageData, rhs.m_imageData, rhs.m_imageWidth * rhs.m_imageHeight * sizeof(unsigned short));
    m_imageWidth = rhs.m_imageWidth;
    m_imageHeight = rhs.m_imageHeight;
    m_exposureTime = rhs.m_exposureTime;

    m_jpegData = rhs.m_jpegData;
    sz_jpegData = rhs.sz_jpegData;
    JpegQuality = rhs.JpegQuality;
    pixelMin = rhs.pixelMin;
    pixelMax = rhs.pixelMax;
    autoscale = rhs.autoscale;
}

CImageData &CImageData::operator=(const CImageData &rhs)
{
    if (&rhs == this)
    { // self asignment
        return *this;
    }

    ClearImage();

    if ((rhs.m_imageWidth == 0) || (rhs.m_imageHeight == 0) || (rhs.m_imageData == 0))
    {
        return *this;
    }

    m_imageData = new unsigned short[rhs.m_imageWidth * rhs.m_imageHeight];

    if (m_imageData == 0)
    {
        return *this;
    }

    memcpy(m_imageData, rhs.m_imageData, rhs.m_imageWidth * rhs.m_imageHeight * sizeof(unsigned short));
    m_imageWidth = rhs.m_imageWidth;
    m_imageHeight = rhs.m_imageHeight;
    return *this;
}

CImageData::~CImageData()
{
    if (m_imageData != NULL)
        delete[] m_imageData;
    if (m_jpegData != nullptr)
        delete[] m_jpegData;
}

ImageStats CImageData::GetStats() const
{
    if (!m_imageData)
    {
        return ImageStats(0, 0, 0, 0);
    }

    int min = 0xFFFF;
    int max = 0;
    double mean = 0.0;

    // Calculate min max and mean

    unsigned short *imageDataPtr = m_imageData;
    double rowDivisor = m_imageHeight * m_imageWidth;
    unsigned long rowSum;
    double rowAverage;
    unsigned short currentPixelValue;

    int rowIndex;
    int columnIndex;

    for (rowIndex = 0; rowIndex < m_imageHeight; rowIndex++)
    {
        rowSum = 0L;

        for (columnIndex = 0; columnIndex < m_imageWidth; columnIndex++)
        {
            currentPixelValue = *imageDataPtr;

            if (currentPixelValue < min)
            {
                min = currentPixelValue;
            }

            if (currentPixelValue > max)
            {
                max = currentPixelValue;
            }

            rowSum += currentPixelValue;

            imageDataPtr++;
        }

        rowAverage = static_cast<double>(rowSum) / rowDivisor;

        mean += rowAverage;
    }

    // Calculate standard deviation

    double varianceSum = 0.0;
    imageDataPtr = m_imageData;

    for (rowIndex = 0; rowIndex < m_imageHeight; rowIndex++)
    {
        for (columnIndex = 0; columnIndex < m_imageWidth; columnIndex++)
        {
            double tempValue = (*imageDataPtr) - mean;
            varianceSum += tempValue * tempValue;
            imageDataPtr++;
        }
    }

    double stddev = sqrt(varianceSum / static_cast<double>((m_imageWidth * m_imageHeight) - 1));

    return ImageStats(min, max, mean, stddev);
}

void CImageData::Add(const CImageData &rhs)
{
    unsigned short *sourcePixelPtr = rhs.m_imageData;
    unsigned short *targetPixelPtr = m_imageData;
    unsigned long newPixelValue;

    if (!rhs.HasData())
        return;

    // if we don't have data yet we simply copy the rhs data
    if (!this->HasData())
    {
        *this = rhs;
        return;
    }

    // we do have data, make sure our size matches the new size
    if ((rhs.m_imageWidth != m_imageWidth) || (rhs.m_imageHeight != m_imageHeight))
        return;

    for (int pixelIndex = 0;
         pixelIndex < (m_imageWidth * m_imageHeight);
         pixelIndex++)
    {
        newPixelValue = *targetPixelPtr + *sourcePixelPtr;

        if (newPixelValue > 0xFFFF)
        {
            *targetPixelPtr = 0xFFFF;
        }
        else
        {
            *targetPixelPtr = static_cast<unsigned short>(newPixelValue);
        }

        sourcePixelPtr++;
        targetPixelPtr++;
    }

    m_exposureTime += rhs.m_exposureTime;

    if (convert_jpeg)
        ConvertJPEG();
}

void CImageData::ApplyBinning(int binX, int binY)
{
    if (!HasData())
        return;
    if ((binX == 1) && (binY == 1))
    { // No binning to apply
        return;
    }

    short newImageWidth = GetImageWidth() / binX;
    short newImageHeight = GetImageHeight() / binY;

    short binSourceImageWidth = newImageWidth * binX;
    short binSourceImageHeight = newImageHeight * binY;

    unsigned short *newImageData = new unsigned short[newImageHeight * newImageWidth];

    memset(newImageData, 0, newImageHeight * newImageWidth * sizeof(unsigned short));

    // Bin the data into the new image space allocated
    for (int rowIndex = 0; rowIndex < binSourceImageHeight; rowIndex++)
    {
        const unsigned short *sourceImageDataPtr = GetImageData() + (rowIndex * GetImageWidth());

        for (int columnIndex = 0; columnIndex < binSourceImageWidth; columnIndex++)
        {
            unsigned short *targetImageDataPtr = newImageData + (((rowIndex / binY) * newImageWidth) +
                                                                 (columnIndex / binX));

            unsigned long newPixelValue = *targetImageDataPtr + *sourceImageDataPtr;

            if (newPixelValue > 0xFFFF)
            {
                *targetImageDataPtr = 0xFFFF;
            }
            else
            {
                *targetImageDataPtr = static_cast<unsigned short>(newPixelValue);
            }

            sourceImageDataPtr++;
        }
    }

    delete[] m_imageData;
    m_imageData = newImageData;
    m_imageWidth = newImageWidth;
    m_imageHeight = newImageHeight;

    if (convert_jpeg)
        ConvertJPEG();
}

void CImageData::FlipHorizontal()
{
    for (int row = 0; row < m_imageHeight; ++row)
    {
        std::reverse(m_imageData + row * m_imageWidth, m_imageData + (row + 1) * m_imageWidth);
    }

    if (convert_jpeg)
        ConvertJPEG();
}

#include <stdint.h>

uint16_t CImageData::DataMin()
{
    uint16_t res = 0xffff;
    if (!HasData())
        return 0xffff;
    int idx = m_imageWidth * m_imageHeight;
    while (idx--)
        if (res > m_imageData[idx])
            res = m_imageData[idx];
    return res;
}

uint16_t CImageData::DataMax()
{
    uint16_t res = 0;
    if (!HasData())
        return 0;
    int idx = m_imageWidth * m_imageHeight;
    while (idx--)
        if (res < m_imageData[idx])
            res = m_imageData[idx];
    return res;
}

#include <stdio.h>

void CImageData::ConvertJPEG()
{
    // Check if data exists
    if (!HasData())
        return;
    // source raw image
    uint16_t *imgptr = m_imageData;
    // temporary bitmap buffer
    uint8_t *data = new uint8_t[m_imageWidth * m_imageHeight * 3]; // 3 channels for RGB
    // autoscale
    uint16_t min, max;
    if (autoscale)
    {
        min = DataMin();
        max = DataMax();
    }
    else
    {
        min = pixelMin;
        max = pixelMax < 0 ? 0xffff : (pixelMax > 0xffff ? 0xffff : pixelMax);
    }
    // scaling
    float scale = 0xffff / ((float)(max - min));
    // Data conversion
    for (int i = 0; i < m_imageWidth * m_imageHeight; i++) // for each pixel in raw image
    {
        int idx = 3 * i;         // RGB pixel in JPEG source bitmap
        if (imgptr[i] == 0xffff) // saturation
        {
            data[idx + 0] = 0xff;
            data[idx + 1] = 0x0;
            data[idx + 2] = 0x0;
        }
        else if (imgptr[i] > max) // higher
        {
            data[idx + 0] = 0xff;
            data[idx + 1] = 0xa5;
            data[idx + 2] = 0x0;
        }
        else // scaling
        {
            uint8_t tmp = (imgptr[i] - min) * scale / 0x100;
            data[idx + 0] = tmp;
            data[idx + 1] = tmp;
            data[idx + 2] = tmp;
        }
    }
    // JPEG output buffer, has to be larger than expected JPEG size
    if (m_jpegData != nullptr)
    {
        delete[] m_jpegData;
        m_jpegData = nullptr;
    }
    m_jpegData = new uint8_t[m_imageWidth * m_imageHeight * 4 + 1024]; // extra room for JPEG conversion
    // JPEG parameters
    jpge::params params;
    params.m_quality = JpegQuality;
    params.m_subsampling = static_cast<jpge::subsampling_t>(2); // 0 == grey, 2 == RGB
    // JPEG compression and image update
    if (!jpge::compress_image_to_jpeg_file_in_memory(m_jpegData, sz_jpegData, m_imageWidth, m_imageHeight, 3, data, params))
    {
        printf("Failed to compress image to jpeg in memory\n");
    }
}

void CImageData::GetJPEGData(unsigned char *&ptr, int &sz)
{
    if (!convert_jpeg)
    {
        convert_jpeg = true;
        ConvertJPEG();
    }
    ptr = m_jpegData;
    sz = sz_jpegData;
}