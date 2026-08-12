
#include <Arduino.h>
#include <SD.h>
#include <MTP_Teensy.h>
#include <Controller.h>
#include <Util.h>

#include <MTP.h>

const int SD_ChipSelect = BUILTIN_SDCARD;

namespace SdMTP
{

    void setup(void)
    {
        // Add the SD Card
        if (SD.begin(SD_ChipSelect))
        {
            Serial.println("SD Card initialized");
            // CACHED_PATTERN_LOAD(abstract_gradient);
            // CACHED_PATTERN_LOAD(blue_light_rays);
            // CACHED_PATTERN_LOAD(color_roll);
            // CACHED_PATTERN_LOAD(fire);
            // CACHED_PATTERN_LOAD(flash);
            // CACHED_PATTERN_LOAD(matrix);
            // CACHED_PATTERN_LOAD(rainbow);
            // CACHED_PATTERN_LOAD(space_warp);

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
