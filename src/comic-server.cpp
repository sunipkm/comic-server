#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <iostream>
#include <queue>

#include "comic-server.hpp"

volatile sig_atomic_t done = 0;
void sig_handler(int sig)
{
#define SIG_PRINT(x)                                  \
    case x:                                           \
    {                                                 \
        printf("Received signal " #x ", exiting..."); \
        fflush(stdout);                               \
        break;                                        \
    }
    switch (sig)
    {
        SIG_PRINT(SIGINT)
        SIG_PRINT(SIGILL)
        SIG_PRINT(SIGABRT)
        SIG_PRINT(SIGFPE)
        SIG_PRINT(SIGSEGV)
        SIG_PRINT(SIGTERM)
        SIG_PRINT(SIGQUIT)
        SIG_PRINT(SIGTRAP)
        SIG_PRINT(SIGBUS)
        SIG_PRINT(SIGSYS)
        SIG_PRINT(SIGPIPE)
        SIG_PRINT(SIGALRM)
    }
    done = 1;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sig_handler);

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

void *CameraThread(void *_inout)
{
    while (!done && cam->CameraReady())
    {
        long retrycount = 1;
        uint64_t start = getTime();
        image = cam->CaptureImage(retrycount);
        if (image.HasData())
        {
            uint8_t *jpg_ptr = nullptr;
            int jpg_sz;
            image.GetJPEGData(jpg_ptr, jpg_sz);
            if (jpg_sz <= 0 || jpg_ptr == nullptr)
            {
                dbprintlf("Invalid JPEG image");
            }
            else
            {
                netimg_meta metadata[1];
                metadata->exposure_ms = cam->GetExposure() * 1000; // exposure in ms
                metadata->width = image.GetImageWidth();
                metadata->height = image.GetImageHeight();
                metadata->tstamp = getTime();
                metadata->size = jpg_sz;
                metadata->type = netimg_type::JPEGRGB;
                uint8_t *buf = new uint8_t[sizeof(netimg_meta) + jpg_sz];
                memcpy(buf, metadata, sizeof(netimg_meta));
                memcpy(buf + sizeof(netimg_meta), jpg_ptr, jpg_sz);
                NetFrame *frame = nullptr;
                frame = new NetFrame(buf, sizeof(netimg_meta) + jpg_sz, comic_netdata::DATA, NetType::DATA, FrameStatus::NONE, 0);
                // queue the netframe
                delete(buf);
            }
        }
        uint64_t end = getTime();
        if ((end - start) < ExposureCadenceMs)
        {
            usleep((ExposureCadenceMs - (end - start)) * 1000);
        }
        else
        {
            usleep(10 * 1000);
        }
    }
    return (void *)1;
}