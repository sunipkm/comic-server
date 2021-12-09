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
        float exposure = cam->GetExposure();
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
                frame = new NetFrame(buf, sizeof(netimg_meta) + jpg_sz, comic_netdata::DATA, NetType::DATA, FrameStatus::NONE, CtrlVertex);
                tx_queue.push(frame);
                delete (buf);
            }
            MainImage->Reset();
            CriticalSection::Lock lock(MainImage->cs);
            MainImage->size = image.GetImageHeight() * image.GetImageWidth() * sizeof(uint16_t);
            MainImage->data = new uint16_t[MainImage->size];
            MainImage->height = image.GetImageHeight();
            MainImage->width = image.GetImageWidth();
            MainImage->exposure_ms = image.GetImageExposure() * 1000;
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

void SaveFits(char *filePrefix, char *DirPrefix, int i, int n, raw_image *image)
{
    static char defaultFilePrefix[] = "comic";
    static char defaultDirPrefix[] = ".\\fits\\";
    if ((filePrefix == NULL) || (strlen(filePrefix) == 0))
        filePrefix = defaultFilePrefix;
    if ((DirPrefix == NULL) || (strlen(DirPrefix) == 0))
        DirPrefix = defaultFilePrefix;
    char fileName[256];
    fitsfile *fptr;
    int status = 0, bitpix = USHORT_IMG, naxis = 2;
    int bzero = 32768, bscale = 1;
    long naxes[2] = {(long)(image->width), (long)(image->height)};
    char *fileName_s = nullptr;
    if (_snprintf(fileName, sizeof(fileName), "%s\\%s_%ums_%d_%d_%llu.fit", DirPrefix, filePrefix, image->exposure_ms, i, n, image->tstamp) > sizeof(fileName))
        goto print_err;
    fileName_s = new char[strlen(fileName) + 16];
    unlink(fileName);
    _snprintf(fileName_s, strlen(fileName) + 16, "%s[compress]", fileName);
    if (!fits_create_file(&fptr, fileName_s, &status))
    {
        fits_create_img(fptr, bitpix, naxis, naxes, &status);
        fits_write_key(fptr, TSTRING, "PROGRAM", (void *)"hitmis_explorer", NULL, &status);
        fits_write_key(fptr, TSTRING, "CAMERA", (void *)cam->CameraName(), NULL, &status);
        fits_write_key(fptr, TULONGLONG, "TIMESTAMP", &(image->tstamp), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BZERO", &bzero, NULL, &status);
        fits_write_key(fptr, TUSHORT, "BSCALE", &bscale, NULL, &status);
        fits_write_key(fptr, TFLOAT, "CCDTEMP", &(image->ccdtemp), NULL, &status);
        fits_write_key(fptr, TUINT, "EXPOSURE_MS", &(image->exposure_ms), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BINX", &(image->binX), NULL, &status);
        fits_write_key(fptr, TUSHORT, "BINY", &(image->binY), NULL, &status);
        fits_write_key(fptr, TUSHORT, "MINX", &(image->x_min), NULL, &status);
        fits_write_key(fptr, TUSHORT, "MAXX", &(image->x_max), NULL, &status);
        fits_write_key(fptr, TUSHORT, "MINY", &(image->y_min), NULL, &status);
        fits_write_key(fptr, TUSHORT, "MAXY", &(image->y_max), NULL, &status);

        long fpixel[] = {1, 1};
        fits_write_pix(fptr, TUSHORT, fpixel, (image->width) * (image->height), image->data, &status);
        fits_close_file(fptr, &status);
        _snprintf(SaveFitsStatus, sizeof(SaveFitsStatus), "saved %d of %d", i, n);
        delete[] fileName_s;
        return;
    }
    delete[] fileName_s;
print_err:
{
    _snprintf(SaveFitsStatus, sizeof(SaveFitsStatus), "failed %d of %d", i, n);
}
}

#ifndef OS_Windows
#include <sys/types.h>
#include <sys/stat.h>
#endif

bool dirExists(const char *dirName)
{
#ifdef OS_Windows
    DWORD ftyp = GetFileAttributesA(dirName);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false; //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true; // this is a directory!

    return false; // this is not a directory!
#else
    struct stat st = {
        0,
    };

    if (stat(dirName, &st) == -1)
        return false;
    if (S_ISDIR(st.st_mode))
        return true;
    return false;
#endif
}

#ifndef OS_Windows
#include <errno.h>
#define ERROR_ALREADY_EXISTS EEXIST
#define ERROR_PATH_NOT_FOUND 6557
int CreateDirectoryA(const char *path, unsigned int * _permissions)
{
    mode_t perm = 0777;
    if (_permissions != NULL)
        perm = *_permissions;
    
    if (mkdir(path, perm) == 0)
        return 1;
    else
        return 0;
}

int GetLastError()
{
    if (errno == EEXIST)
        return ERROR_ALREADY_EXISTS;
    if (errno == ENOENT)
        return ERROR_PATH_NOT_FOUND;
    if (errno == ENOTDIR)
        return ERROR_PATH_NOT_FOUND;
    if (errno == EFAULT)
        return ERROR_PATH_NOT_FOUND;
    if (errno == EACCES)
        return ERROR_PATH_NOT_FOUND;
    return 0;
}
#endif