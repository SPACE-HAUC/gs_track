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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <TargetSystem.hpp>
#include "clkgen.h"
#include "meb_debug.h"
#include <signal.h>

volatile sig_atomic_t done = 0;

void sighandler(int sig)
{
    done = 1;
}

int main(int argc, char *argv[])
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

    strcpy(global->TLE1, DEFAULT_TLE1);
    strcpy(global->TLE2, DEFAULT_TLE2);

    // Create GSN thread IDs.
    pthread_t net_polling_tid, net_rx_tid;

    // TODO: Send a network packet with the current AzEl data each time it is updated. Example below.
    // { AZ-EL TRACKING DATA REPORT EXAMPLE
    //     int AzEl[2] = {0};
    //     AzEl[0] = azimuth;
    //     AzEl[1] = elevation;
    //     NetFrame *AzElFrame = new NetFrame((unsigned char *) AzEl, sizeof(AzEl), NetType::TRACKING_DATA, NetVertex::CLIENT);
    //     AzElFrame->sendFrame(global->netdata);
    //     delete AzElFrame;
    // }

    TargetSystem tsys;
    if (argc > 2)
    {
        dbprintlf(FATAL "Invalid number of command-line arguments given.");
        return -1;
    }
    else if (argc == 2)
    {
        tsys.Create(argv[1], TLE1, TLE2, 42.65578686304611, -71.32546893568428, 1);
    }
    else
        tsys.Create(TLE1, TLE2, 42.65578686304611, -71.32546893568428, 8);

    clkgen_t clk = create_clk(1000000000, tsys.TimerHandler, &tsys);

    printf("Running tracker, Ctrl+C to exit\n");

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

        CoordTopocentric coord = tsys.GetPosition();
        CoordGeodetic geo = tsys.GetGeoPosition();
        printf("Target location: %d %d | %3.2lf %3.2lf %3.2lf\n", (int)coord.azimuth, (int)coord.elevation, geo.latitude, geo.longitude, geo.altitude);

        // Send target location through the network.
        track_data_t *info = {0};
        info.az = (int)coord.azimuth;
        info.el = (int)coord.elevation;
        info.lat = geo.latitude;
        info.lon = geo.longitude;
        info.alt = geo.altitude;
        NetFrame *dataFrame = new NetFrame((unsigned char *) info, sizeof(track_data_t), NetType::TRACKING_DATA, NetVertex::CLIENT);
        dataFrame->sendFrame(global->netdata);
        delete dataFrame;
        sleep(1);
    }

    destroy_clk(clk);

    return 0;
}