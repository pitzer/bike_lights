// Controller.cpp
//
// Main entry point - provides Arduino's setup() and loop() functions which
// tie together all the main functionality of the different modules.

#include <Arduino.h>
#include <Controller.h>
#include <Heartbeat.h>
#include <Util.h>
#include <LED.h>
#include <SdMTP.h>
#include <Persist.h>
#include <GPS.h>

void setup() {

    Serial.begin(115200);
    Serial.println("Begin");

    Heartbeat::setup();
    Util::setup();
    Persist::setup();
    SdMTP::setup();
    LED::setup();
    GPS::setup();
    
    Serial.println("BranchController Setup Complete");
}



void loop() {
    Heartbeat::loop();
    // SdMTP::loop();
    LED::loop();
    GPS::loop();
}