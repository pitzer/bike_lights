#pragma once

#include <Arduino.h>

#define HEARTBEAT_PERIOD_MS 1700
constexpr uint8_t pinHeartbeat = LED_BUILTIN;

namespace Heartbeat {
    void setup(void);
    void loop(void); 
}
