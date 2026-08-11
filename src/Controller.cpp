// Controller.cpp
//
// Main entry point - provides Arduino's setup() and loop() functions which
// tie together all the main functionality of the different modules.

#include <Arduino.h>
#include <Controller.h>
#include <Heartbeat.h>
#include <Util.h>
#include <LED.h>
#include <Persist.h>

void setup() {

    Serial.println("Begin");

    Heartbeat::setup();
    Util::setup();
    Persist::setup();
    LED::setup();
    
    Serial.println("BranchController Setup Complete");
}



void loop() {
    Heartbeat::loop();
    LED::loop();
}