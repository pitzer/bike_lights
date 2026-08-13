#pragma once

#include <Arduino.h>

namespace GPS {
    void setup(void);
    void loop(void); 
    void setTeensyTimeFromGPS();
    void displayInfo();
    uint64_t absoluteTimeMs();
}
