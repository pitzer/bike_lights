#include "led_string.h"
#include <EEPROM.h>

uint32_t current_channel = 0;

static const uint16_t magic = 0xCAFE;

static const uint8_t default_color_ordering = WS2811_GRB;
static const uint32_t default_pattern_index = 0;
static const CRGB default_single_color = CRGB::Red;
static const int32_t default_palette_index = 0;
static const int32_t default_update_period_ms = 3000;
static const uint8_t default_brightness = 255;

CRGB leds_crgb[max_leds_per_channel];

void led_array_init()
{
}

void led_array_save()
{
    // Write the magic
    EEPROM.write(0, magic & 0xFF);
    EEPROM.write(1, magic >> 8);
    // Write the size of the string array to validate the stored data on load.
    uint16_t size = sizeof(led_string_t) * num_strings;
    EEPROM.write(2, size & 0xFF);
    EEPROM.write(3, size >> 8);

    uint8_t *data = reinterpret_cast<uint8_t *>(led_strings);
    for (uint16_t i = 0; i < size; ++i)
    {
        EEPROM.write(4 + i, data[i]);
    }
}

void led_array_load()
{
    // Read the magic
    uint16_t magic_read = EEPROM.read(0) | (EEPROM.read(1) << 8);
    if (magic_read != magic)
    {
        return;
    }
    // Read the size
    uint16_t size = EEPROM.read(2) | (EEPROM.read(3) << 8);
    if (size != sizeof(led_string_t) * num_strings)
    {
        return;
    }

    uint8_t *data = reinterpret_cast<uint8_t *>(led_strings);
    for (uint16_t i = 0; i < size; ++i)
    {
        data[i] = EEPROM.read(4 + i);
    }
}