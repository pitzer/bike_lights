#include <GPS.h>
#include <TimeLib.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

static const int RXPin = 0, TXPin = 1;
static const uint32_t GPSBaud = 115200;
static uint64_t epochOffsetMs = 0;
static bool epochOffsetValid = false;

TinyGPSPlus gps;

// Use Serial1 on Teensy
// #define GPSSerial Serial1

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

namespace GPS
{
    void setup(void)
    {
        ss.begin(GPSBaud);
    }

    void loop(void)
    {
        // Read incoming bytes from GPS
        while (ss.available() > 0)
        {
            if (gps.encode(ss.read()))
            {
                if (gps.date.isValid() && gps.time.isValid() && gps.time.isUpdated() && gps.time.age() < 500)
                {
                    setTeensyTimeFromGPS();
                    displayInfo();
                }
            }
        }
    }

    uint64_t absoluteTimeMs()
    {
        if (!epochOffsetValid)
        {
            epochOffsetMs = (uint64_t)now() * 1000ULL - (uint64_t)millis();
            epochOffsetValid = true;
        }
        return epochOffsetMs + (uint64_t)millis();
    }

    void setTeensyTimeFromGPS()
    {
        // Extract UTC time and date
        int year = gps.date.year();
        int month = gps.date.month();
        int day = gps.date.day();
        int hour = gps.time.hour();
        int minute = gps.time.minute();
        int second = gps.time.second();

        // Set the Time library / system time provider
        setTime(hour, minute, second, day, month, year);

        // Sync the hardware RTC built into the Teensy
        Teensy3Clock.set(now());

        // Update the epoch offset for absolute time calculations
        epochOffsetMs = (uint64_t)now() * 1000ULL - (uint64_t)millis();
        epochOffsetValid = true;
    }

    void displayInfo()
    {
        Serial.print(F("Location: "));
        if (gps.location.isValid())
        {
            Serial.print(gps.location.lat(), 6);
            Serial.print(F(","));
            Serial.print(gps.location.lng(), 6);
        }
        else
        {
            Serial.print(F("INVALID"));
        }

        Serial.print(F("  Date/Time: "));
        if (gps.date.isValid())
        {
            Serial.print(gps.date.month());
            Serial.print(F("/"));
            Serial.print(gps.date.day());
            Serial.print(F("/"));
            Serial.print(gps.date.year());
        }
        else
        {
            Serial.print(F("INVALID"));
        }

        Serial.print(F(" "));
        if (gps.time.isValid())
        {
            if (gps.time.hour() < 10)
                Serial.print(F("0"));
            Serial.print(gps.time.hour());
            Serial.print(F(":"));
            if (gps.time.minute() < 10)
                Serial.print(F("0"));
            Serial.print(gps.time.minute());
            Serial.print(F(":"));
            if (gps.time.second() < 10)
                Serial.print(F("0"));
            Serial.print(gps.time.second());
            Serial.print(F("."));
            if (gps.time.centisecond() < 10)
                Serial.print(F("0"));
            Serial.print(gps.time.centisecond());
        }
        else
        {
            Serial.print(F("INVALID"));
        }

        Serial.println();
        // Serial.printf("millis() = %lu, Teensy3Clock.get() = %lu, now() = %lu\n", millis(), Teensy3Clock.get(), now());
    }
}