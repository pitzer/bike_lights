#ifndef LED_PATTERN_H
#define LED_PATTERN_H

#include <FastLED.h>
#include <SD.h>

// Struct to describe a pattern file resource.
struct __attribute__((packed)) pattern_file_header_t
{
    // Two bytes indicating pattern format.
    uint16_t magic;
    // The color ordering used by the pattern.
    uint8_t color_ordering;
    // The number of pixels.
    uint16_t num_pixels;
    // The number of animation steps stored.
    uint16_t animation_steps;
    // The nominal animation period in seconds.
    uint16_t animation_period_s;
};

typedef struct
{
    pattern_file_header_t header;
    // Size of the image in bytes.
    uint32_t data_size;
    const char *filepath;
    File file;
} pattern_file_t;

// Macro to load a pattern file from SD.
#define PATTERN_FILE_LOAD(p)                                                      \
    p.file = SD.open(p.filepath);                                                 \
    if (p.file)                                                                   \
    {                                                                             \
        p.file.read(&p.header, sizeof(p.header));                                 \
        Serial.printf("magic: %X\\n", p.header.magic);                           \
        Serial.printf("color_ordering: %d\\n", p.header.color_ordering);         \
        Serial.printf("num_pixels: %d\\n", p.header.num_pixels);                 \
        Serial.printf("animation_steps: %d\\n", p.header.animation_steps);       \
        Serial.printf("animation_period_s: %d\\n", p.header.animation_period_s); \
    }                                                                             \
    else                                                                          \
    {                                                                             \
        Serial.println("Failed to open file from SD card.");                     \
    }

// Static function to count total number of LEDs addressed by the pattern.
constexpr uint32_t total_pattern_leds(const uint32_t num_segments,
                                      const uint32_t *num_leds_per_segment,
                                      std::size_t i = 0U)
{
    return i < num_segments ? (num_leds_per_segment[i] +
                               total_pattern_leds(num_segments,
                                                  num_leds_per_segment,
                                                  i + 1U))
                            : 0;
}

// Macro to validate data consistency.
#define CHECK_PATTERN_FILE(p)                                       \
    constexpr uint32_t num_leds =                                   \
        total_pattern_leds(p.num_segments, p.num_leds_per_segment); \
    constexpr uint32_t num_pixels = p.animation_steps * num_leds;   \
    static_assert(num_pixels == p.num_pixels);

// An LED pattern function
typedef void (*led_pattern_func_t)(
    // The current time, in ms
    uint32_t time_ms,
    // The period to use for the pattern, in ms
    uint32_t period_ms,
    // The palette to use to render the LEDs. Can be
    // potentially ignored by the pattern if it wants to use a single color
    const CRGBPalette16 *palette,
    // The single color to use for the pattern, If it wants a single color
    CRGB single_color,
    uint32_t string_index,
    uint32_t segment_index,
    // The number of LEDs in the string
    uint32_t num_leds,
    // The array of LEDs to update
    CRGB *leds);

// This struct is used to describe an LED pattern, which will drive a string of LEDs
typedef struct
{
    // The name of the pattern
    const char *name;
    // The description of the pattern
    const char *desc;
    // The function that will be called to update the LEDs
    led_pattern_func_t update;
} led_pattern_t;

// All the available LED patterns
extern led_pattern_t led_patterns[];

// Number of patterns available
extern uint32_t num_led_patterns();

void pattern_file(pattern_file_t *pattern,
                  uint32_t time_ms,
                  uint32_t string_index,
                  uint32_t segment_index,
                  uint32_t num_leds,
                  CRGB *leds);


#endif // LED_PATTERN_H
