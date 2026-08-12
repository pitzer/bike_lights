#include "led_pattern.h"
#include "led_string.h"
#include "generated_led_patterns.h"
#include <Arduino.h>

// Set all the LEDs to the palette color, regardless of time
void static_pattern(uint32_t time_ms, uint32_t period_ms, const CRGBPalette16 *palette, CRGB single_color, uint32_t string_index, uint32_t segment_index, uint32_t num_leds, CRGB *leds)
{
    for (uint32_t i = 0; i < num_leds; i++)
    {
        uint8_t palette_index = i * 255 / num_leds;
        leds[i] = ColorFromPalette(*palette, palette_index);
    }
}

// Rotate all the LEDs on a palette
void rotate_pattern(uint32_t time_ms, uint32_t period_ms, const CRGBPalette16 *palette, CRGB single_color, uint32_t string_index, uint32_t segment_index, uint32_t num_leds, CRGB *leds)
{
    uint32_t offset = 255 - time_ms * 255 / period_ms;
    for (uint32_t i = 0; i < num_leds; i++)
    {
        uint8_t palette_index = i * 255 / num_leds + offset;
        leds[i] = ColorFromPalette(*palette, palette_index);
    }
}

// Fade all the LEDs on a palette
void fade_pattern(uint32_t time_ms, uint32_t period_ms, const CRGBPalette16 *palette, CRGB single_color, uint32_t string_index, uint32_t segment_index, uint32_t num_leds, CRGB *leds)
{
    uint32_t fade_u32 = (time_ms * 512 / period_ms) % 512;
    if (fade_u32 >= 256)
    {
        fade_u32 = 511 - fade_u32;
    }
    uint8_t fade_u8 = fade_u32;
    for (uint32_t i = 0; i < num_leds; i++)
    {
        uint8_t palette_index = i * 255 / num_leds;
        leds[i] = ColorFromPalette(*palette, palette_index, fade_u8);
    }
}

// Blink all the LEDs on a palette
void blink_pattern(uint32_t time_ms, uint32_t period_ms, const CRGBPalette16 *palette, CRGB single_color, uint32_t string_index, uint32_t segment_index, uint32_t num_leds, CRGB *leds)
{
    bool on = time_ms % period_ms < period_ms / 2;
    for (uint32_t i = 0; i < num_leds; i++)
    {
        uint8_t palette_index = i * 255 / num_leds;
        if (on)
        {
            leds[i] = ColorFromPalette(*palette, palette_index);
        }
        else
        {
            leds[i] = CRGB::Black;
        }
    }
}


void pattern_file(pattern_file_t* pattern, uint32_t time_ms, uint32_t string_index, uint32_t segment_index, uint32_t num_leds, CRGB *leds)
{
    led_string_t *led_string = &led_strings[string_index];
    const uint32_t period_ms = pattern->header.animation_period_s * 1000;
    const uint32_t step = (time_ms * pattern->header.animation_steps / period_ms) % pattern->header.animation_steps;

    // Seek to offset where to start reading LED data
    uint64_t pos = sizeof(pattern_file_header_t);
    pos += pattern->header.num_pixels * step * sizeof(CRGB);
    for (uint32_t i = 0; i < string_index; i++) {
        pos += led_strings[i].num_leds * sizeof(CRGB);
    }
    for (uint32_t i = 0; i < segment_index; i++) {
        pos += led_string->segments[i].num_leds * sizeof(CRGB);
    }
    pattern->file.seek(pos);
    // Serial.printf("step: %d, pos: %" PRIu64 "\n", step, pos);

    // Read LED data for segment.
    for (uint32_t i = 0; i < num_leds; i++)
    {
        pattern->file.read(&leds[i], sizeof(CRGB));
    }
}

uint32_t num_led_patterns()
{
    return generated_num_led_patterns;
}
