/**
 * @file track.cpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.09.13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "track.hpp"
#include "network.hpp"
#include "meb_debug.hpp"

void *gs_network_rx_thread(void *args)
{
    global_data_t *global = (global_data_t *)args;
    NetDataClient *netdata = global->netdata;

    while (netdata->recv_active && netdata->thread_status > 0)
    {
        if (!netdata->connection_ready)
        {
            usleep(5 SEC);
            continue;
        }

        int read_size = 0;

        while (read_size >= 0 && netdata->recv_active && netdata->thread_status > 0)
        {
            dbprintlf(BLUE_BG "Waiting to receive...");

            NetFrame *netframe = new NetFrame();
            read_size = netframe->recvFrame(netdata);

            dbprintlf("Read %d bytes.", read_size);

            if (read_size >= 0)
            {
                dbprintlf("Received the following NetFrame:");
                netframe->print();
                netframe->printNetstat();

                // Extract the payload into a buffer.
                // Safe only because recvFrame return PAYLOAD SIZE.
                int payload_size = netframe->getPayloadSize();
                unsigned char *payload = (unsigned char *)malloc(payload_size);
                if (payload == nullptr)
                {
                    dbprintlf(FATAL "Memory for payload failed to allocate, packet lost.");
                    continue;
                }

                if (netframe->retrievePayload(payload, payload_size) < 0)
                {
                    dbprintlf(RED_FG "Error retrieving data.");
                    if (payload != nullptr)
                    {
                        free(payload);
                        payload = nullptr;
                    }
                    continue;
                }

                switch (netframe->getType())
                {
                case NetType::TRACKING_COMMAND:
                {
                    dbprintlf(BLUE_FG "Received a TRACKING COMMAND frame.");

                    track_cmd_t *command = (track_cmd_t *) payload;
                    
                    // UPDATE TLE CMD
                    if (command->cmd == CMD_UPDATE_TLE)
                    {
                        strcpy(command->TLE1, global->TLE1);
                        strcpy(command->TLE2, global->TLE2);

                        dbprintlf(BLUE_FG "TLE updated to:\n%s\n%s", global->TLE1, global->TLE2);
                    }

                    break;
                }
                case NetType::ACK:
                {
                    dbprintlf(BLUE_FG "Received an ACK frame.");
                    break;
                }
                case NetType::NACK:
                {
                    dbprintlf(BLUE_FG "Received a NACK frame.");
                    break;
                }
                default:
                {
                }
                    if (payload != nullptr)
                    {
                        free(payload);
                        payload = nullptr;
                    }
                }
            }
            else
            {
                break;
            }

            delete netframe;
        }
        if (read_size == -404)
        {
            dbprintlf(RED_BG "Connection forcibly closed by the server.");
            strcpy(netdata->disconnect_reason, "SERVER-FORCED");
            netdata->connection_ready = false;
            continue;
        }
        else if (errno == EAGAIN)
        {
            dbprintlf(YELLOW_BG "Active connection timed-out (%d).", read_size);
            strcpy(netdata->disconnect_reason, "TIMED-OUT");
            netdata->connection_ready = false;
            continue;
        }
        erprintlf(errno);
    }

    dbprintlf(FATAL "GS_NETWORK_RX_THREAD IS EXITING!");
    if (global->netdata->thread_status > 0)
    {
        global->netdata->thread_status = 0;
    }
}