#include "RingBuf.hpp"
#include <unistd.h>
#include <iostream>
#include <curses.h>
#include "clkgen.h"
#include "CameraUnit_ATIK.hpp"
#include "meb_print.h"
#include <math.h>

typedef struct
{
    // inputs
    CCameraUnit *cam;       // camera
    float Temp_Target;      // Temperature target
    float Temp_Rate_Target; // Temperature rate target
    float Time_Rate;        // Timer delta
    float Kp;
    float Ki;
    float Kd;
    // outputs
    int runcount;
    float T;
    float dT;
    int sleepTimeUs;
    // readonly
    bool reset;
    // lock
    pthread_mutex_t lock;
    // log file
    FILE *fp;
} ThermalPID_Data;

void ThermalPID_Control(clkgen_t id, void *_pid_data);

#define CREATE_NCUR_WINDOW(name) \
    WINDOW *win_##name;          \
    int x_##name, y_##name, w_##name, h_##name;

CREATE_NCUR_WINDOW(data);
CREATE_NCUR_WINDOW(opts);

void print_menu(WINDOW *, int);

static std::string choices[] = {
    "Set Temperature Target",          // 0
    "Set Temperature Gradient Target", // 1
    "Set Time Rate",                   // 2
    "Set Kp",                          // 3
    "Set Ki",                          // 4
    "Set Kd",                          // 5
    "Quit"                             // 6
};

static int n_choices = sizeof(choices) / sizeof(choices[0]);

#include <signal.h>
volatile sig_atomic_t done = 0;
void sighandler(int sig)
{
    exit(0);
}

void curses_cleanup()
{
    clear();
    endwin();
}

static char clearline[512];

void DrawClearLine()
{
    int i = 0;
    for (i = 0; i < COLS - 2; i++)
    {
        clearline[i] = ' ';
    }
    clearline[i] = '\0';
}

void curses_init()
{
    static bool runOnce = false;
    if (!runOnce)
    {
        runOnce = true;
        atexit(curses_cleanup);
    }
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);

    w_data = COLS;
    h_data = 6;
    x_data = 0;
    y_data = 0; // top of the window

    w_opts = COLS;
    h_opts = LINES - h_data;
    x_opts = 0;
    y_opts = h_data;

    if (COLS < 80 || LINES < (h_data + n_choices + 6) || COLS > 500)
    {
        mvprintw(0, 0, "Minimum terminal size 80 x %d, you have %d x %d\nPress any key to exit...", h_data + n_choices + 6, COLS, LINES);
        getch();
        endwin();
        exit(0);
    }

    win_data = newwin(h_data, w_data, y_data, x_data);
    win_opts = newwin(h_opts, w_opts, y_opts, x_opts);
    keypad(win_opts, TRUE);
    DrawClearLine();
}

int main(int argc, char *argv[])
{
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
    bprintlf("Camera: %s\tTemperature: %.2f C", cam->CameraName(), cam->GetTemperature());
    double exposure = 0.2;
    if (argc == 2)
    {
        exposure = atof(argv[1]);
        if (exposure < 0.001)
            exposure = 0.001;
        if (exposure > 20)
            exposure = 20;
    }
    cam->SetBinningAndROI(1, 1);
    cam->SetExposure(exposure);
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
    if (fp != NULL)
    {
        fwrite(jpg_ptr, 1, jpg_sz, fp);
        fclose(fp);
    }
    exit(0);
    curses_init();
    ThermalPID_Data pid_data[1];
    memset(pid_data, 0x0, sizeof(ThermalPID_Data));
    pid_data->fp = fopen("templog.txt", "w+");
    pthread_mutex_init(&(pid_data->lock), NULL);
    pid_data->cam = cam;
    // Start timer at 20 ms clock (default)
    pid_data->Time_Rate = 1;                                                                   // 20 ms
    clkgen_t clk = create_clk(pid_data->Time_Rate * 1000000000LLU, ThermalPID_Control, pid_data); // unsigned long long
    int c, select = 0, choice = 0, choice_made = 0;
    while (!done)
    {
        print_menu(win_opts, select);
        c = wgetch(win_opts);
        switch (c)
        {
        case KEY_UP:
            if (select == 0)
                select = n_choices - 1;
            else
                --select;
            break;

        case KEY_DOWN:
            if (select == n_choices - 1)
                select = 0;
            else
                ++select;
            break;

        case '\n': // selection
            choice = select;
            choice_made = 1;
            break;

        case KEY_RESIZE:
            curses_cleanup();
            curses_init();
            wclear(win_data);
            box(win_data, 0, 0);
            break;

        default:
            break;
        }
        if (choice_made)
        {
            choice_made = 0;
            wclear(win_opts);
            switch (choice)
            {
            case 0:
            {
                float tmp;
                wprintw(win_opts, "Temperature target (C): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(tmp));
                noecho();
                if (tmp < -20)
                {
                    tmp = -20;
                    wprintw(win_opts, "Warning: Temperature setting below -20 is not supported.\nPress any key to continue...");
                    wgetch(win_opts);
                }
                pthread_mutex_lock(&(pid_data->lock));
                pid_data->Temp_Target = tmp;
                pthread_mutex_unlock(&(pid_data->lock));
                wprintw(win_opts, "Temp target updated, press any key to continue...");
                wgetch(win_opts);

                break;
            }
            case 1:
            {
                float tmp;
                wprintw(win_opts, "Temperature Gradient target (C/s): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(tmp));
                noecho();
                if (tmp > 0)
                {
                    tmp = 0;
                    wprintw(win_opts, "Warning: Temperature gradient above 0 is not supported.\nPress any key to continue...");
                    wgetch(win_opts);
                    break;
                }
                if (tmp < -1)
                {
                    tmp = -1;
                    wprintw(win_opts, "Warning: Temperature gradient below -1 C/s is not supported.\nPress any key to continue...");
                    wgetch(win_opts);
                    break;
                }
                pthread_mutex_lock(&(pid_data->lock));
                pid_data->Temp_Rate_Target = tmp;
                pthread_mutex_unlock(&(pid_data->lock));
                wprintw(win_opts, "Temp gradient target updated, press any key to continue...");
                wgetch(win_opts);

                break;
            }
            case 2:
            {
                destroy_clk(clk);
                wprintw(win_opts, "Time Rate Target (per second): ");
                wrefresh(win_opts);
                echo();
                int num_run = 1;
                wscanw(win_opts, " %d", &(num_run));
                noecho();
                if (num_run > 50)
                    num_run = 50;
                else if (num_run <= 1)
                    num_run = 1;
                pid_data->Time_Rate = 1.0 / num_run;
                pid_data->reset = true;
                clk = create_clk(1000 * 1000 * 1000 / num_run, ThermalPID_Control, pid_data);
                break;
            }
            case 3:
            {
                float tmp;
                wprintw(win_opts, "Kp: ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(tmp));
                noecho();
                pthread_mutex_lock(&(pid_data->lock));
                pid_data->Kp = tmp;
                pthread_mutex_unlock(&(pid_data->lock));
                wprintw(win_opts, "Kp updated, press any key to continue...");
                wgetch(win_opts);

                break;
            }
            case 4:
            {
                float tmp;
                wprintw(win_opts, "Ki (s^-1): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(tmp));
                noecho();
                pthread_mutex_lock(&(pid_data->lock));
                pid_data->Ki = tmp;
                pthread_mutex_unlock(&(pid_data->lock));
                wprintw(win_opts, "Ki updated, press any key to continue...");
                wgetch(win_opts);

                break;
            }
            case 5:
            {
                float tmp;
                wprintw(win_opts, "Kd (s): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(tmp));
                noecho();
                pthread_mutex_lock(&(pid_data->lock));
                pid_data->Kd = tmp;
                pthread_mutex_unlock(&(pid_data->lock));
                wprintw(win_opts, "Kd updated, press any key to continue...");
                wgetch(win_opts);

                break;
            }
            case 6:
            {
                destroy_clk(clk);
                done = 1;
                // gpioWrite
            }
            }
        }
    }
    fclose(pid_data->fp);
    exit(0);
}

void print_menu(WINDOW *win, int sel)
{
    wclear(win);
    box(win, 0, 0);
    int x = 2, y = 2;
    mvwprintw(win, y, x, "Press up/down arrow keys to highlight a menu and press enter to select.");
    y += 2;
    for (int i = 0; i < n_choices; i++)
    {
        if (sel == i) // Highlight the present choice
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, y, x, "%d. %s", i + 1, choices[i].c_str());
            wattroff(win, A_REVERSE);
        }
        else
        {
            mvwprintw(win, y, x, "%d. %s", i + 1, choices[i].c_str());
        }
        y++;
    }
    wrefresh(win);
}

static inline char *get_time_now_str()
{
#ifndef _MSC_VER
    static __thread char buf[128];
#else
    __declspec( thread ) static char buf[128];
#endif
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

void ThermalPID_Control(clkgen_t id, void *_pid_data)
{
    static float err = 0.0f, // current delta
        prev_err = 0.0f,     // previous delta
        i_err = 0.0f;        // integral error
    static float mes_oldold = 0.0f,
                 mes_old = 0.0f;
    static int runcount = 0;
    static bool ready = false;
    static bool firstRun = true;
    ThermalPID_Data *pid_data = (ThermalPID_Data *)_pid_data;
    // reset logic
    if (pid_data->reset)
    {
        firstRun = true;
        ready = false;
        err = 0;
        prev_err = 0;
        i_err = 0;
        runcount = 0;
        pid_data->reset = false;
    }
    // Draw box for first run
    if (firstRun)
    {
        wclear(win_data);
        box(win_data, 0, 0);
        firstRun = false;
    }
    // Clear lines
    mvwprintw(win_data, 1, 1, "%s", clearline);
    mvwprintw(win_data, 2, 1, "%s", clearline);
    mvwprintw(win_data, 3, 1, "%s", clearline);
    mvwprintw(win_data, 4, 1, "%s", clearline);
    // 1. Make measurement
    float mes = pid_data->cam->GetTemperature();
    if (pid_data->fp != NULL)
    {
        fprintf(pid_data->fp, "[%s] %.2f\n", get_time_now_str(), mes);
        fflush(pid_data->fp);
    }
    ++runcount;
    // Try to lock PID data, time sensitive
    if (pthread_mutex_trylock(&(pid_data->lock)))
    {
        // store measurement and move on
        mes_oldold = mes_old;
        mes_old = mes;
        wrefresh(win_data);
        if (runcount == 2)
            ready = true;
        return;
    }
    pid_data->T = mes;
    pid_data->runcount = runcount;
    // Print Run Info
    mvwprintw(win_data, 1, 1, "Run: %.2f s  Temp: %.2f C  Target: %.2f C Grad: %.2f C/s", runcount * pid_data->Time_Rate, mes, pid_data->Temp_Target, pid_data->Temp_Rate_Target);
    // 2. Calculate Required Time Rate
    float Temp_Rate_Target = pid_data->Temp_Rate_Target;
    if ((mes - pid_data->Temp_Target) < 1) // less than 1 degree
        Temp_Rate_Target = (mes - pid_data->Temp_Target) / pid_data->Time_Rate;
    if (fabs(Temp_Rate_Target) < 1e-6)
        Temp_Rate_Target = 0;
    long long int sleepTime = 0;
    if (ready)
    {
        // 3. Calculate Active Time Rate
        float dT = (mes - mes_oldold) / (2 * pid_data->Time_Rate);
        pid_data->dT = dT;
        prev_err = err;
        err = Temp_Rate_Target - dT;
        i_err += err;                // integral error
        float derr = err - prev_err; // derivative error

        float P = pid_data->Kp * err;
        float I = pid_data->Ki * i_err * pid_data->Time_Rate;
        float D = pid_data->Kd * derr / pid_data->Time_Rate;

        sleepTime = (P + I + D) * 1e6;
        pid_data->sleepTimeUs = sleepTime;
        sleepTime = (sleepTime % ((int)(pid_data->Time_Rate * 1e6)));
        mvwprintw(win_data, 2, 1, "dT = %.3e | P %.3e I %.3e D %.3e", dT, P, I, D);
        mvwprintw(win_data, 3, 1, "Kp = %.2e Ki = %.2e Kd = %.2e", pid_data->Kp, pid_data->Ki, pid_data->Kd);
        mvwprintw(win_data, 4, 1, "Actuate = %d us", sleepTime);
    }
    else if (runcount == 2)
    {
        ready = true;
    }
    // 5. Update old data
    mes_oldold = mes_old;
    mes_old = mes;
    // refresh window
    wrefresh(win_data);
    // Unlock pid_data
    pthread_mutex_unlock(&(pid_data->lock));
    // 4. Actuate
    if (sleepTime <= 1500)
    {
        // do nothing
    }
    else
    {
        sleepTime -= 1000;
        usleep(sleepTime);
    }
}