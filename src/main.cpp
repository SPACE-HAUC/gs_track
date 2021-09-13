/**
 * @file main.cpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.09.13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include "network.hpp"
#include "meb_debug.hpp"
#include <pthread.h>
#include "track.hpp"

#define lower_Az 40
#define upper_Az 110
#define lower_El 10
#define upper_El 35

volatile sig_atomic_t done = 0;
void sighandler(int sig)
{
    done = 1;
}

// Generates AzEl data for a fake satellite pass.

int main()
{
    // Ignores broken pipe signal, which is sent to the calling process when writing to a nonexistent socket (
    // see: https://www.linuxquestions.org/questions/programming-9/how-to-detect-broken-pipe-in-c-linux-292898/
    // and 
    // https://github.com/sunipkmukherjee/example_imgui_server_client/blob/master/guimain.cpp
    // Allows manual handling of a broken pipe signal using 'if (errno == EPIPE) {...}'.
    // Broken pipe signal will crash the process, and it caused by sending data to a closed socket.
    signal(SIGPIPE, SIG_IGN);

    // Set up netdata.
    global_data_t global[1] = {0};
    global->netdata = new NetDataClient(NetPort::TRACK, SERVER_POLL_RATE);
    global->netdata->recv_active = true;

    // Create GSN thread IDs.
    pthread_t net_polling_tid, net_rx_tid;

    signal(SIGINT, sighandler);
    int AzEl[2] = {0};
    bool upward = true;
    bool iteratedLast = false;

    int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 3)
    {
        printf("Error opening /dev/ttyUSB0, check wiring and sudo\n");
        return 0;
    }
    struct termios options[1];
    tcgetattr(fd, options);
    options->c_cflag = B2400 | CS8 | CLOCAL | CREAD;
    options->c_iflag = IGNPAR;
    options->c_oflag = 0;
    options->c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, options);

    // FILE *fp = fopen("data.txt", "w");

    // TODO: Currently uses the simulation (fake tracking) code. Update to using actual TLE data.

    char cmd[50];
    ssize_t sz;
    sz = sprintf(cmd, "PB %d\r", lower_Az);
    if (write(fd, cmd, sz) < sz)
    {
        printf("Error writing azimuth command\n");
    }
    sleep(1);
    sz = sprintf(cmd, "PA %d\r", lower_El);
    if (write(fd, cmd, sz) < sz)
    {
        printf("Error writing elevation command\n");
    }
    sleep(30); 
    while (!done)
    {
        // Not initialized or recoverable failure - thread boot required.
        if (global->netdata->thread_status == 0)
        {
            if (global->netdata->connection_ready)
            {
                dbprintlf(YELLOW_FG "(Re)Booting network receive and poll threads.");
                pthread_create(&net_polling_tid, NULL, gs_polling_thread, global->netdata);
                pthread_create(&net_rx_tid, NULL, gs_network_rx_thread, global);
            }
            else 
            {
                dbprintlf(RED_FG "Cannot reboot network receive and poll threads, connection not ready.");
            }
        }

        // TODO: Send a network packet with the current AzEl data each time it is updated. Example below.
        // { AZ-EL TRACKING DATA REPORT EXAMPLE
        //     int AzEl[2] = {0};
        //     AzEl[0] = azimuth;
        //     AzEl[1] = elevation;
        //     NetFrame *AzElFrame = new NetFrame((unsigned char *) AzEl, sizeof(AzEl), NetType::TRACKING_DATA, NetVertex::CLIENT);
        //     AzElFrame->sendFrame(global->netdata);
        //     delete AzElFrame;
        // }
        

        for (int az = lower_Az; (az <= upper_Az) && (!done); az++)
        {
            double idx = (az - lower_Az) * M_PI / 70; // 0 -- pi
            int el = (upper_El - lower_El) * sin(idx) + lower_El;
            printf("Az: %d deg, El: %d deg\n", az, el);
            
            sz = snprintf(cmd, sizeof(cmd), "PB %d\r", az);
            if (write(fd, cmd, sz) < sz)
            {
                printf("Error writing azimuth command\n");
            }
            sleep(1);
            sz = snprintf(cmd, sizeof(cmd), "PA %d\r", el);
            if (write(fd, cmd, sz) < sz)
            {
                printf("Error writing elevation command\n");
            }
            // if (!iteratedLast)
            //     fprintf(fp, "%d %d\n", az, el);
            sleep(1);
        }
        sz = sprintf(cmd, "PB %d\r", lower_Az);
        if (write(fd, cmd, sz) < sz)
        {
            printf("Error writing azimuth command\n");
        }
        sz = sprintf(cmd, "PA %d\r", lower_El);
        if (write(fd, cmd, sz) < sz)
        {
            printf("Error writing elevation command\n");
        }
        // if (iteratedLast == false)
        // {
        //     iteratedLast = true;
        //     fclose(fp);
        // }
        for (int i = 0; (i < 60) && (!done); i++)
            sleep(1);
    }
    sleep(1);
    sz = sprintf(cmd, "PB %d\r", 0);
    if (write(fd, cmd, sz) < sz)
    {
        printf("Error writing azimuth command\n");
    }
    sz = sprintf(cmd, "PA %d\r", 90);
    sleep(1);
    if (write(fd, cmd, sz) < sz)
    {
        printf("Error writing elevation command\n");
    }
    close(fd);
    return 0;
}