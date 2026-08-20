#pragma once

// Code for driving 8 LED strips in parallel using FastLED's
// OctoWS2811 support
//
// Best documentation: https://github.com/FastLED/FastLED/wiki/Parallel-Output

#include <Arduino.h>
#include <Controller.h>
#include <OctoWS2811.h>

// The number of LED channels that we are managing.
static constexpr uint32_t num_led_channels = 8;
// The maximum number of LEDs per LED string.
static constexpr uint32_t max_leds_per_channel = 512;
// The total number of LEDs.
static constexpr uint32_t max_leds = num_led_channels * max_leds_per_channel;

namespace LED {

    enum Pattern { patternSolid = 0, patternTest  };    // more  patterns can be added here

    void setup();
    void load_persistant_data();
    void loop();
    void advancePattern();

    void setSolidColor(int rgb);
    void setPixel(int strip, int led, int rgb);
    void setPixel(int strip, int led, uint8_t r, uint8_t g, uint8_t b);
    void testPattern();
    bool togglePower();
    void CalculateFrameRate();
    void show();

}