#include "CameraUnit_ATIK.hpp"
#include "jpge.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

int main()
{
    CCameraUnit *cam = nullptr;
    long retryCount = 10;
    do
    {
        cam = new CCameraUnit_ATIK();
        if (cam->CameraReady())
        {
            std::cout << "Camera ready\n";
            break;
        }
        else
        {
            delete cam;
            cam = nullptr;
        }
    } while (retryCount--);
    if (cam == nullptr)
    {
        std::cout << "Error opening camera\n";
        exit(0);
    }
    std::cout << "Camera: " << cam->CameraName() << "\tTemperature: " << cam->GetTemperature() << " C" << std::endl;
    cam->SetBinningAndROI(1, 1);
    cam->SetExposure(0.2);
    CImageData img = cam->CaptureImage(retryCount);
    if (!img.HasData())
    {
        std::cout << "Image data empty\n";
        exit(0);
    }
    unsigned char *jpg_ptr;
    int jpg_sz;
    img.GetJPEGData(jpg_ptr, jpg_sz);
    FILE *fp = fopen("test.jpg", "wb");
    fwrite(jpg_ptr, 1, jpg_sz, fp);
    fclose(fp);
    exit(0);
}