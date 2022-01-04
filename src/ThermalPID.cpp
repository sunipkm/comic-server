#include "RingBuf.hpp"
#include <unistd.h>
#include <iostream>
#include <curses.h>
#include "clkgen.h"
#include "CameraUnit_ATIK.hpp"
#include "meb_print.h"

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
    curses_init();
    ThermalPID_Data pid_data[1];
    memset(pid_data, 0x0, sizeof(ThermalPID_Data));
    pid_data->cam = cam;
    // Start timer at 20 ms clock (default)
    pid_data->Time_Rate = 0.02; // 20 ms
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
                wprintw(win_opts, "Temperature target (C): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(pid_data->Temp_Target));
                noecho();
                break;
            }
            case 1:
            {
                wprintw(win_opts, "Temperature Gradient target (C/s): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(pid_data->Temp_Rate_Target));
                noecho();
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
                wprintw(win_opts, "Kp: ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(pid_data->Temp_Rate_Target));
                noecho();
                break;
            }
            case 4:
            {
                wprintw(win_opts, "Ki (s^-1): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(pid_data->Temp_Rate_Target));
                noecho();
                break;
            }
            case 5:
            {
                wprintw(win_opts, "Kd (s): ");
                wrefresh(win_opts);
                echo();
                wscanw(win_opts, " %f", &(pid_data->Temp_Rate_Target));
                noecho();
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
    // 1. Make measurement
    float mes = pid_data->cam->GetTemperature();
    pid_data->T = mes;
    pid_data->runcount = runcount;
    // 2. Calculate Required Time Rate
    float Temp_Rate_Target = pid_data->Temp_Rate_Target;
    if ((mes - pid_data->Temp_Target) < 1) // less than 1 degree
        Temp_Rate_Target = (mes - pid_data->Temp_Target) / pid_data->Time_Rate;
    if (Temp_Rate_Target < 1e-6)
        Temp_Rate_Target = 0;
    ++runcount;

    if (firstRun)
    {
        wclear(win_data);
        box(win_data, 0, 0);
        firstRun = false;
    }
    mvwprintw(win_data, 1, 1, "%s", clearline);
    mvwprintw(win_data, 2, 1, "%s", clearline);
    mvwprintw(win_data, 3, 1, "%s", clearline);
    mvwprintw(win_data, 1, 1, "Run: %.2f s Temp: %.2f C", runcount * pid_data->Time_Rate, mes);
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

        int sleepTime = (P + I + D) * 1e6;
        pid_data->sleepTimeUs = sleepTime;
        sleepTime = (sleepTime % ((int)(pid_data->Time_Rate * 1e6)));
        mvwprintw(win_data, 2, 1, "dT = %.3e | P %.3e I %.3e D %.3e", dT, P, I, D);
        mvwprintw(win_data, 3, 1, "Actuate = %d us", sleepTime);
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
    else if (runcount == 2)
    {
        ready = true;
    }
    // 5. Update old data
    mes_oldold = mes_old;
    mes_old = mes;
    // refresh window
    wrefresh(win_data);
}