#include "CameraUnit_ATIK.hpp"
#include "meb_print.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>

void checknmakedir(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "mkdir -p %s", path);
        system(buf);
    }
    else
    {
        dbprintlf(FATAL "could not create directory %s", path);
    }
}

static inline char *get_date()
{
#ifndef _MSC_VER
    static __thread char buf[128];
#else
    __declspec( thread ) static char buf[128];
#endif
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buf, sizeof(buf), "%04d%02d%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

#include <chrono>

static inline long long int getTime()
{
    return ((std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())).time_since_epoch())).count());
}

volatile sig_atomic_t done = 0;
void sighandler(int sig)
{
    done = 1;
}

static inline long long int getSunRiseTime()
{
    FILE *pp = popen("./getsuntimes.py --type sunrise", "r");
    long long int ts = 0;
    if (pp != NULL)
    {
        char buf[128] = {
            0,
        };
        memset(buf, 0x0, sizeof(buf));
        fread(buf, sizeof(buf), 1, pp);
        ts = atoll(buf);
        pclose(pp);
    }
    return ts;
}

static inline long long int getSunSetTime()
{
    FILE *pp = popen("./getsuntimes.py --type sunset", "r");
    long long int ts = 0;
    if (pp != NULL)
    {
        char buf[128] = {
            0,
        };
        memset(buf, 0x0, sizeof(buf));
        fread(buf, sizeof(buf), 1, pp);
        ts = atoll(buf);
        pclose(pp);
    }
    return ts;
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
    // first run, get sunrise and sunset times
    long long int risetime = getSunRiseTime();
    long long int settime = getSunSetTime();
    if (risetime == 0 || settime == 0)
    {
        dbprintlf("Fatal error: Sun set time %lld, Sun rise time %lld", settime, risetime);
    }
    while (!done)
    {
        static char fname[512];
        static char dirname[512];
        static int bin = 1;
        static float exposure = 0.2;
        static long long int timenow = 0;
        static bool exposing = false;
        timenow = getTime();
        if (timenow <= risetime && timenow >= settime) // valid time, take photos
        {
            exposing = true;
            CImageData img = cam->CaptureImage(retryCount);
            snprintf(dirname, sizeof(dirname), "fits/%s", get_date());
            checknmakedir(dirname);
            img.SaveFits(NULL, dirname);
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
        else if (exposing == true)
        {
            risetime = getSunRiseTime();
            settime = getSunSetTime();
            exposing = false;
            usleep(1000000 * cadence);
        }
        else
        {
            tprintf("Time to next sunset: ");
            long long int timedelta = (settime - timenow) / 1000;
            int hours = timedelta / 3600;
            timedelta -= hours * 3600;
            if (hours > 0)
                bprintf("%d hours ", hours);
            int minutes = timedelta / 60;
            timedelta -= minutes * 60;
            if (minutes > 0)
                bprintf("%d minutes ", minutes);
            bprintlf("%d seconds", (int)timedelta);
            usleep(1000000 * cadence);
        }
    }
    exit(0);
}