#pragma once

// Code for driving 8 LED strips in parallel using FastLED's
// OctoWS2811 support
//
// Best documentation: https://github.com/FastLED/FastLED/wiki/Parallel-Output

#include <Arduino.h>
#include <Controller.h>
#include <OctoWS2811.h>


namespace LED {

    enum Pattern { patternSolid = 0, patternTest  };    // more  patterns can be added here

    void setup();
    void load_persistant_data();
    void loop();

    void setSolidColor(int rgb);
    void setPixel(int strip, int led, int rgb);
    void setPixel(int strip, int led, uint8_t r, uint8_t g, uint8_t b);
    void testPattern();
    bool togglePower();
    void openPixelClientConnection(bool f);
    void CalculateFrameRate();
    void show();

}