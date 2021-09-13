/**
 * @file TrackingMotor.hpp
 * @author Sunip K. Mukherjee
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.08.16
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef TRACKING_MOTOR_HPP
#define TRACKING_MOTOR_HPP

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

class TrackingMotor
{
private:
    bool ready;
    int fd;
    int az, el;
    int open_conn(const char *name)
    {
        ready = false;
        if (name == nullptr)
        {
            return -1;
        }

        int conn = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
        if (conn < 3)
        {
            return -1;
        }

        struct termios options[1];
        tcgetattr(conn, options);
        options->c_cflag = B2400 | CS8 | CLOCAL | CREAD;
        options->c_iflag = IGNPAR;
        options->c_oflag = 0;
        options->c_lflag = 0;
        tcflush(conn, TCIFLUSH);
        tcsetattr(conn, TCSANOW, options);

        return conn;
    }

public:
    TrackingMotor()
    {
        fd = -1;
        ready = false;
    }

    TrackingMotor(const char *name)
    {
        if (name == NULL)
            return;
        fd = open_conn(name);
        if (fd >= 3)
            ready = true;
        az = 0;
        el = 90;
        SetAz(az);
        SetEl(el);
    }

    int Open()
    {
        return Open("/dev/ttyUSB0");
    }

    int Open(const char *name)
    {
        if (name == NULL)
            return -1;
        fd = open_conn(name);
        if (fd >= 3)
            ready = true;
        // az = 0;
        // el = 90;
        // SetAz(az);
        // SetEl(el);
        return fd;
    }

    int SetAz(int az)
    {
        static char buf[8];
        if (az < 0)
            return -1;
        if (az > 360)
            return -1;
        if (ready)
        {
            int sz = snprintf(buf, sizeof(buf), "PB %d\r", az);
            int retval = write(fd, buf, sz);
            if (sz == retval)
            {
                this->az = az;
                usleep(0.1 * 1000000);
                return az;
            }
            else
            {
                return -2;
            }    
        }
        return -3;
    }

    int GetAz()
    {
        return this->az;
    }

    int SetEl(int el)
    {
        static char buf[8];
        if (el < 0)
            return -1;
        if (el > 90)
            return -1;
        if (ready)
        {
            int sz = snprintf(buf, sizeof(buf), "PA %d\r", el);
            int retval = write(fd, buf, sz);
            if (sz == retval)
            {
                this->el = el;
                usleep(0.1 * 1000000);
                return el;
            }
            else
            {
                return -2;
            }    
        }
        return -3;
    }

    int GetEl()
    {
        return this->el;
    }

    bool IsReady()
    {
        return ready;
    }

    ~TrackingMotor()
    {
        ready = false;
        if (fd >= 3)
            close(fd);
    }
};
#endif // TRACKING_MOTOR_HPP