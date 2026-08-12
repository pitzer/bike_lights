#include <LED.h>
#include <Util.h>
#include <Persist.h>

namespace LED {
    // Any group of digital pins may be used
    byte pinList[NUM_STRIPS] = {2, 14, 7, 8, 6, 20, 21, 5};


    // These buffers need to be large enough for all the pixels.
    // The total number of pixels is "LEDS_PER_STRIP * NUM_STRIPS".
    // Each pixel needs 3 bytes, so multiply by 3.  An "int" is
    // 4 bytes, so divide by 4.  The array is created using "int"
    // so the compiler will align it to 32 bit memory.
    const int bytesPerLED = 3;  // change to 4 if using RGBW
    DMAMEM int displayMemory[LEDS_PER_STRIP * NUM_STRIPS * bytesPerLED / 4];
    int drawingMemory[LEDS_PER_STRIP * NUM_STRIPS * bytesPerLED / 4];

    const int config = WS2811_RGB | WS2811_800kHz;

    OctoWS2811 leds(LEDS_PER_STRIP, displayMemory, drawingMemory, config, NUM_STRIPS, pinList);


    enum Pattern pattern = patternTest;

    bool fPowerOn = true;
    bool fOpenPixelClientConnected = false;
    int rgbSolidColor;
    uint32_t tmFrameStart;
    unsigned int cFrames;

    int make_color_rgb(unsigned int red, unsigned int green, unsigned int blue)
    {
        if (red > 255) red = 255;
        if (green > 255) green = 255;
        if (blue > 255) blue = 255;
        return (red << 16) | (green << 8) | blue;
    }

    unsigned int h2rgb(unsigned int v1, unsigned int v2, unsigned int hue)
    {
        if (hue < 60) return v1 * 60 + (v2 - v1) * hue;
        if (hue < 180) return v2 * 60;
        if (hue < 240) return v1 * 60 + (v2 - v1) * (240 - hue);
        return v1 * 60;
    }


    // Convert HSL (Hue, Saturation, Lightness) to RGB (Red, Green, Blue)
    //
    //   hue:        0 to 359 - position on the color wheel, 0=red, 60=orange,
    //                            120=yellow, 180=green, 240=blue, 300=violet
    //
    //   saturation: 0 to 100 - how bright or dull the color, 100=full, 0=gray
    //
    //   lightness:  0 to 100 - how light the color is, 100=white, 50=color, 0=black
    //
    int make_color_hsl(unsigned int hue, unsigned int saturation, unsigned int lightness)
    {
        unsigned int red, green, blue;
        unsigned int var1, var2;

        if (hue > 359) hue = hue % 360;
        if (saturation > 100) saturation = 100;
        if (lightness > 100) lightness = 100;

        // algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
        if (saturation == 0) {
            red = green = blue = lightness * 255 / 100;
        } else {
            if (lightness < 50) {
                var2 = lightness * (100 + saturation);
            } else {
                var2 = ((lightness + saturation) * 100) - (saturation * lightness);
            }
            var1 = lightness * 200 - var2;
            red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / 600000;
            green = h2rgb(var1, var2, hue) * 255 / 600000;
            blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / 600000;
        }
        return make_color_rgb(red, green, blue);
    }

    void setup() {

        tmFrameStart = millis();
        cFrames = 0;
        leds.begin();

        load_persistant_data();
    }

    void load_persistant_data() {
        pattern = (enum Pattern) Persist::data.pattern;
        rgbSolidColor = Persist::data.rgbSolidColor;
    }

    void show_color(int color) {
        for (int i=0; i < leds.numPixels(); i++) {
            leds.setPixel(i, color);
        }
        leds.show();
    }

    void setPixel(int strip, int led, int rgb) {
        leds.setPixel((strip*LEDS_PER_STRIP) + led, rgb);
    }

    void setPixel(int strip, int led, uint8_t r, uint8_t g, uint8_t b) {
        setPixel(strip, led, make_color_rgb(r, g, b));
    }

    void loop() {

        if (!fPowerOn)
        {
            show_color(BLACK);;
            return;
        }

        if (fOpenPixelClientConnected)
        {
            // let the protocol drive the LEDs
            return;
        }
        
        if (pattern == patternSolid)
        {
            show_color(rgbSolidColor);
            return;
        }

        // This is the test pattern:

        static uint8_t hue = 0;

        // Serial.printf("%d\n", hue);

        for(int i = 0; i < NUM_STRIPS; i++) {
            for(int j = 0; j < LEDS_PER_STRIP; j++) {
                // setPixel(i, j, make_color_hsl((32*i) + hue+j,100,100));
                setPixel(i, j, make_color_rgb(hue, 0, 0));
            }
        }

        // // Set the first n leds on each strip to show which strip it is
        // for(int i = 0; i < NUM_STRIPS; i++) {
        //     for(int j = 0; j <= i; j++) {
        //         setPixel(i , j, make_color_rgb(0x65,0x43,0x21));
        //     }
        // }

        hue++;

        // instead of calling show(), we call delay() which guarantees to call show()
        // but also gives FastLED a chance to do some temporal dithering.
        leds.show();
        CalculateFrameRate();

    }

    void CalculateFrameRate()
    {
        // frame rate calc
        cFrames++;
        if (millis() > (tmFrameStart + 1000))
        {
            cFrames = 0;
            tmFrameStart = millis();
        }
    }

    void setSolidColor(int rgb) {

        pattern = patternSolid;
        rgbSolidColor = rgb;

        Persist::data.pattern = (uint8_t) pattern;
        Persist::data.rgbSolidColor = rgb;

    }

    void testPattern() {

        pattern = patternTest;
        Persist::data.pattern = (uint8_t) pattern;

    }

    bool togglePower() {

        fPowerOn = !fPowerOn;
        return fPowerOn;

    }

    void show() {
        leds.show();
    }


}
