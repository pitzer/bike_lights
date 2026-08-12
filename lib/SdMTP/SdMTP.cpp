
#include <Arduino.h>
#include <SD.h>
#include <MTP_Teensy.h>
#include <Controller.h>
#include <Util.h>
#include "generated_led_patterns.h"

const int SD_ChipSelect = BUILTIN_SDCARD;

namespace SdMTP
{

    void setup(void)
    {
        // Add the SD Card
        if (SD.begin(SD_ChipSelect))
        {
            Serial.println("SD Card initialized");
            generated_led_patterns_load();

            // Start MTP
            MTP.begin();
            MTP.addFilesystem(SD, "SD_Card");
        }
    }

    void loop(void)
    {
        MTP.loop();
    }

}
