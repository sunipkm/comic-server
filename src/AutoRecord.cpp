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
    float cadence = 0;
    float maxExposure = 0;
    float pixelPercentile = 0;
    int pixelTarget = 0;
    int pixelUncertainty = 0;
    int maxBin = 0;
    int imgX = 0, imgY = 0, imgW = 0, imgH = 0;

    bool triggered = false;

    while ((opt = getopt(argc, argv, "cmptubxywh")) != -1)
    {
        switch (opt)
        {
        case 'c':
            triggered = true;
            cadence = atof(optarg);
            break;
        case 'm':
            triggered = true;
            maxExposure = atof(optarg);
            break;
        case 'p':
            triggered = true;
            pixelPercentile = atof(optarg);
            break;
        case 't':
            triggered = true;
            pixelTarget = atoi(optarg);
            break;
        case 'u':
            triggered = true;
            pixelUncertainty = atoi(optarg);
            break;
        case 'b':
            triggered = true;
            maxBin = atoi(optarg);
            break;
        case 'x':
            triggered = true;
            imgX = atoi(optarg);
            break;
        case 'y':
            triggered = true;
            imgY = atoi(optarg);
            break;
        case 'w':
            triggered = true;
            imgW = atoi(optarg);
            break;
        case 'h':
            triggered = true;
            imgH = atoi(optarg);
            break;
        default:
            bprintlf(RED_FG "Option %c not available", opt);
            break;
        }
    }
    if (!triggered)
    {
        bprintf("Usage:\n\n%s [-c Cadence in s] [-m Max exposure in s] [-p Pixel percentile %%] [-t Pixel target] [-u Pixel uncertainty] [-b Max bin] [-x Image X] [-y Image Y] [-w Image width] [-h Image Height]\n\n", argv[0]);
    }
    if (cadence < 1)
        cadence = 1;
    if (maxExposure < 1)
        maxExposure = 10;
    if (pixelPercentile < 1)
        pixelPercentile = 90;
    if (pixelTarget < 1)
        pixelTarget = 40000;
    if (pixelUncertainty < 1)
        pixelUncertainty = 5000;
    if (maxBin < 1)
        maxBin = 4;
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
    cam->SetBinningAndROI(1, 1, imgX, imgY, imgW, imgH);
    while (!done)
    {
        static int bin = 1;
        static float exposure = 0.2;
        CImageData img = cam->CaptureImage(retryCount);
        img.SaveFits("cam", NULL);
        img.FindOptimumExposure(exposure, bin, pixelPercentile, pixelTarget, maxExposure, maxBin, 100, pixelUncertainty);
        cam->SetBinningAndROI(bin, bin, imgX, imgY, imgW, imgH);
        cam->SetExposure(exposure);
        long int sleeptime = 1000000 * (cadence - exposure);
        if (sleeptime > 0)
            usleep(sleeptime);
    }
    exit(0);
}