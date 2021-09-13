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

typedef struct
{
    NetDataClient *netdata;
    char TLE1[64];
    char TLE2[64];
} global_data_t;

typedef struct
{
    int cmd;
    char TLE1[64];
    char TLE2[64];
} track_cmd_t;

void *gs_network_rx_thread(void *args);

#endif // TRACK_HPP