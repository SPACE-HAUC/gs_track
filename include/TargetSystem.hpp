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

#ifndef TARGET_SYSTEM_HPP
#define TARGET_SYSTEM_HPP

#include <TrackingMotor.hpp>
#include <SGP4/CoordTopocentric.h>
#include <SGP4/Observer.h>
#include <SGP4/SGP4.h>
#include <pthread.h>
#include "meb_debug.h"
#include "clkgen.h"

#define LOOKAHEAD_TIME 60 // Seconds

class TargetSystem
{
private:
    SGP4 *sgp = nullptr;
    Observer *obs;
    TrackingMotor mot;
    pthread_mutex_t lock;
    bool ready = false;
    bool targetVisible = true;
    bool targetPrimed = false;
    int elevation_min = floor(0 * M_PI / 180);

public:
    TargetSystem() : obs(new Observer(0, 0, 0))
    {
    }

    int Create(const char *devname, const char *TLE1, const char *TLE2, double lat, double lon, double alt)
    {
        memset(&lock, 0x0, sizeof(pthread_mutex_t));
        mot = TrackingMotor();
        int retval = 0;
        if ((TLE1 == NULL) || (TLE2 == NULL))
            return -2;
        if (devname == NULL)
            retval = mot.Open();
        else
            retval = mot.Open(devname);
        if (retval < 0)
            return -1;
        Tle tle = Tle(TLE1, TLE2);
        obs->SetLocation(CoordGeodetic(lat, lon, alt));
        sgp = new SGP4(tle);
        ready = true;
        return 1;
    }
    int Create(const char *TLE1, const char *TLE2, double lat, double lon, double alt)
    {
        memset(&lock, 0x0, sizeof(pthread_mutex_t));
        mot = TrackingMotor();
        int retval = 0;
        if ((TLE1 == NULL) || (TLE2 == NULL))
            return -2;
        retval = mot.Open();
        if (retval < 0)
            return -1;
        Tle tle = Tle(TLE1, TLE2);
        obs->SetLocation(CoordGeodetic(lat, lon, alt));
        sgp = new SGP4(tle);
        ready = true;
        return 1;
    }
    int UpdateTLE(const char *TLE1, const char *TLE2)
    {
        if ((TLE1 != NULL) && (TLE2 != NULL))
        {
            pthread_mutex_lock(&lock);
            Tle tle = Tle(TLE1, TLE2);
            sgp = new SGP4(tle);
            pthread_mutex_unlock(&lock);
            return 1;
        }
        else
            return -1;
    }
    int UpdateObs(double lat, double lon, double alt)
    {
        pthread_mutex_lock(&lock);
        obs->SetLocation(CoordGeodetic(lat, lon, alt));
        pthread_mutex_unlock(&lock);
        return 1;
    }
    CoordTopocentric GetPosition()
    {
        DateTime dt = DateTime::Now(true);                                     // current time
        Eci eci = sgp->FindPosition(dt);
        CoordTopocentric coord = obs->GetLookAngle(eci);
        coord.elevation *= 180 / M_PI;
        coord.azimuth *= 180 / M_PI;
        return coord;
    }
    CoordGeodetic GetGeoPosition()
    {
        DateTime dt = DateTime::Now(true);                                     // current time
        Eci eci = sgp->FindPosition(dt);
        CoordGeodetic geocoord = eci.ToGeodetic();
        geocoord.latitude *= 180 / M_PI;
        geocoord.longitude *= 180 / M_PI;
        return geocoord;
    }
    int Track()
    {
        int retval = 0;
        if (ready)
        {
            pthread_mutex_lock(&lock);
            DateTime dt = DateTime::Now(true);                                     // current time
            Eci eci = sgp->FindPosition(dt);                                       // find position now
            CoordTopocentric coord = obs->GetLookAngle(eci);                       // find look angle
#ifdef TARGET_SYS_DEBUG
            dbprintlf("Target: %d %d, visible: %s", (int)(coord.azimuth * 180 / M_PI), (int)(coord.elevation * 180 / M_PI), targetVisible ? "YES" : "NO ");
#endif
            if (targetVisible) // target is already visible
            {
                if (coord.elevation < elevation_min) // outside of look zone
                {
                    targetVisible = false; // mark target invisible
                    retval = 1;
                    mot.SetEl(90); // parked position
                    goto ret;
                }
                int retval = 0;
                if ((int)(coord.azimuth * 180 / M_PI) != mot.GetAz()) // set azimuth
                    retval = mot.SetAz(coord.azimuth * 180 / M_PI);
                if (retval < 0)
                    dbprintlf("Error setting azimuth, %d", retval);
                if ((int)(coord.elevation * 180 / M_PI) != mot.GetEl()) // set elevation
                    mot.SetEl(coord.elevation * 180 / M_PI);
                if (retval < 0)
                    dbprintlf("Error setting elevation, %d", retval);

                retval = 1;
                goto ret;
            }
            else if (targetPrimed) // target primed, not tracking yet
            {
                if (coord.elevation >= elevation_min)
                    targetVisible = true;
                goto ret;
            }
            else // search when, within the lookahead time, it will come over horizon and move to that target
            {
                DateTime dt2 = dt.AddSeconds(LOOKAHEAD_TIME);
                eci = sgp->FindPosition(dt2);
                coord = obs->GetLookAngle(eci);
                int seconds = 0;
                do
                {
                    dt = dt.AddSeconds(1);
                    seconds++;
                    eci = sgp->FindPosition(dt);
                    coord = obs->GetLookAngle(eci);
                } while ((coord.elevation < elevation_min) && (seconds < LOOKAHEAD_TIME));
                if (coord.elevation >= elevation_min)
                {
                    targetPrimed = true;
                    mot.SetEl(coord.elevation);
                    mot.SetAz(coord.azimuth);
                }
#ifdef TARGET_SYS_DEBUG
                dbprintlf("Target: %d %d after %d seconds", (int)(coord.azimuth * 180 / M_PI), (int)(coord.elevation * 180 / M_PI), seconds);
#endif
                retval = 1;
                goto ret;
            }
            pthread_mutex_unlock(&lock);
        }
    ret:
        pthread_mutex_unlock(&lock);
        return retval;
    }
    static void TimerHandler(clkgen_t clk, void *p)
    {
        TargetSystem *sys = (TargetSystem *)p;
        sys->Track();
    }
};

#endif