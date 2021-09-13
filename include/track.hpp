/**
 * @file track.hpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.09.13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "network.hpp"

#ifndef TRACK_HPP
#define TRACK_HPP

#define SEC *1000000
#define CMD_UPDATE_TLE 1
#define DEFAULT_TLE1 "1 25544U 98067A   21229.77243765  .00001431  00000-0  34209-4 0  9998"
#define DEFAULT_TLE2 "2 25544  51.6441  38.1681 0001381 320.9423  62.5381 15.48912726298140"

typedef struct
{
    NetDataClient *netdata;
    char TLE1[100];
    char TLE2[100];
} global_data_t;

typedef struct
{
    int cmd;
    char TLE1[64];
    char TLE2[64];
} track_cmd_t;

typedef struct
{
    int az;
    int el;
    double lat;
    double lon;
    double alt;
} track_data_t;

void *gs_network_rx_thread(void *args);

#endif // TRACK_HPP