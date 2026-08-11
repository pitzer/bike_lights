#pragma once

// Code for persisting configuration data to the Teensy EEPROM
//
// When we store things in EEPROM we store two bytes "bC" in the first
// two bytes. If we ever read anything else there, we assume that
// EEPROM is not initialized.
//
// Then we store a persistence_t structure, byte-for-byte. The 
// first uint16_t of the structure contains the size of the structure
// as stored, in bytes.
//

#include <Arduino.h>
#include <Controller.h>


namespace Persist {

    struct persistence_t {

        uint16_t    cb;                 // Must always be sizeof(persistence_t) - for versioning

        int         rgbSolidColor;      // current color to display 
        uint8_t     pattern;            // whether we are in solid color mode (0) or test pattern (1) -- maps to enum Pattern in LED.cpp

        bool        static_ip;          // false (default) = use DHCP. true = IP address in following field
        byte        ip_addr[4];         // IP address for static IP
        byte        mask[4];         // IP address for static IP
        byte        gateway[4];         // IP address for static IP
        float       center_orientation; // To calibrate the orientation of head facing towards the center
    };

    extern persistence_t data;
    
    void setup();
    void write();

}