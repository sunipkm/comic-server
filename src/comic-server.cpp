#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <queue>

#include "CameraUnit_ATIK.hpp"

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
}