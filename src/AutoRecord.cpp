#include "CameraUnit_ATIK.hpp"
#include "meb_print.h"
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t done = 0;
void sighandler(int sig)
{
    done = 1;
}

int main(int argc, char *argv[])
{
    int opt;
    float cadence = 10;
    float maxExposure = 120;
    float pixelPercentile = 90;
    int pixelTarget = 40000;
    int pixelUncertainty = 5000;
    int maxBin = 4;
    int imgXMin = 100, imgYMin = 335, imgXMax = -1, imgYMax = -1;

    signal(SIGINT, sighandler);
    CCameraUnit *cam = nullptr;
    long retryCount = 10;
    do
    {
        cam = new CCameraUnit_ATIK();
        if (cam->CameraReady())
        {
            bprintlf(GREEN_FG "Camera ready");
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
        bprintlf(RED_FG "Error opening camera");
        exit(0);
    }
    cam->SetExposure(0.2);
    cam->SetBinningAndROI(1, 1, imgXMin, imgXMax, imgYMin, imgYMax);
    unsigned long long counter = 0;
    while (!done)
    {
        static char fname[512];
        static int bin = 1;
        static float exposure = 0.2;
        CImageData img = cam->CaptureImage(retryCount);
        img.SaveFits(NULL, NULL);
        img.FindOptimumExposure(exposure, bin, pixelPercentile, pixelTarget, maxExposure, maxBin, 100, pixelUncertainty);
        cam->SetBinningAndROI(bin, bin, imgXMin, imgXMax, imgYMin, imgYMax);
        cam->SetExposure(exposure);
        if (argc > 1)
        {
            unsigned char *ptr;
            int sz;
            img.GetJPEGData(ptr, sz);
            snprintf(fname, sizeof(fname), "fits/%llu.jpg", ++counter);
            FILE *fp = fopen(fname, "wb");
            fwrite(ptr, sz, 1, fp);
            fclose(fp);
        }
        long int sleeptime = 1000000 * (cadence - exposure);
        if (sleeptime > 0)
            usleep(sleeptime);
    }
    exit(0);
}