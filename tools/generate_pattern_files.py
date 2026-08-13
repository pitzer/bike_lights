import argparse
import json
import asyncio
import os
import re
import struct
import sys
from aiofile import async_open

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LOCAL_FUNKY_LIGHTS = os.path.abspath(os.path.join(ROOT, "..", "funky_lights", "controller"))


def _add_local_funky_lights_to_path():
    if os.path.isdir(LOCAL_FUNKY_LIGHTS):
        if LOCAL_FUNKY_LIGHTS not in sys.path:
            sys.path.insert(0, LOCAL_FUNKY_LIGHTS)
        return True
    return False


def _import_pattern_packages():
    try:
        from controller.core.pattern_cache import PatternCache
        from controller.patterns import pattern_config
        return PatternCache, pattern_config
    except ModuleNotFoundError:
        pass

    try:
        from funky_lights.controller.core.pattern_cache import PatternCache
        from funky_lights.controller.patterns import pattern_config
        return PatternCache, pattern_config
    except ModuleNotFoundError:
        pass

    if _add_local_funky_lights_to_path():
        try:
            from controller.core.pattern_cache import PatternCache
            from controller.patterns import pattern_config
            return PatternCache, pattern_config
        except ModuleNotFoundError:
            pass

        try:
            from funky_lights.controller.core.pattern_cache import PatternCache
            from funky_lights.controller.patterns import pattern_config
            return PatternCache, pattern_config
        except ModuleNotFoundError:
            pass

    raise ImportError(
        "Could not import pattern packages from funky_lights. "
        "Install the package with requirements.txt or ensure the local funky_lights repo exists at: {}".format(
            LOCAL_FUNKY_LIGHTS
        )
    )

PatternCache, pattern_config = _import_pattern_packages()


WS2811_RGB	= 0	
WS2811_RBG	= 1
WS2811_GRB	= 2
WS2811_GBR	= 3
WS2811_BRG	= 4
WS2811_BGR	= 5


def resolve_pattern_config_path(root, path):
    current = root
    for part in path.split('.'):
        if not part:
            raise ValueError(f"Invalid pattern config path '{path}'")

        if isinstance(current, dict):
            if part not in current:
                raise ValueError(f"Pattern configuration '{path}' is not available")
            current = current[part]
            continue

        if not hasattr(current, part):
            raise ValueError(f"Pattern configuration '{path}' is not available")
        current = getattr(current, part)

    return current


def normalize_pattern_set(selected, config_name):
    if isinstance(selected, dict):
        return [selected]

    if isinstance(selected, (list, tuple)):
        for i, item in enumerate(selected):
            if not isinstance(item, dict):
                raise TypeError(
                    f"Pattern configuration '{config_name}' contains non-dict item at index {i}"
                )
        return list(selected)

    raise TypeError(
        f"Pattern configuration '{config_name}' must resolve to a dict or list/tuple of dicts"
    )


class PatternGenerator:
    def __init__(self, pattern_config, led_config, animation_rate, folder, controller_folder=None):
        self.patterns = {}
        self.pattern_config = pattern_config
        self.led_config = led_config
        self.animation_rate = animation_rate
        self.folder = folder
        self.controller_folder = controller_folder

    def patterns_for_caching(self):
        for d in self.pattern_config:
            for pattern_id, _ in d.items():
                yield pattern_id

    def _sanitize_identifier(self, value):
        value = re.sub(r'[^0-9a-zA-Z_]', '_', value)
        if re.match(r'^[0-9]', value):
            value = '_' + value
        return value

    def _segment_variable_name(self, index, name):
        return f"generated_segments_{index}_{self._sanitize_identifier(name)}"

    def _zone_name(self, segment_name):
        if '/' in segment_name:
            return segment_name.split('/')[0]
        return segment_name

    def _string_group_key(self, segment, fallback_index):
        for key in ('string_index', 'string_id', 'string', 'strip', 'channel', 'output_channel'):
            if key in segment:
                return segment[key]
        return fallback_index

    def _string_channel(self, segment, fallback_index):
        for key in ('channel', 'output_channel', 'string_index', 'strip'):
            if key in segment:
                return segment[key]
        return fallback_index

    def _string_name(self, segment, fallback_index):
        for key in ('string_name', 'strip_name'):
            if key in segment:
                return segment[key]
        return segment.get('name', f'string_{fallback_index}')

    def _pattern_symbol_name(self, pattern_id):
        return self._sanitize_identifier(pattern_id)

    def _pattern_function_name(self, pattern_id):
        return f"generated_{self._pattern_symbol_name(pattern_id)}_pattern"

    def _pattern_display_name(self, pattern_id):
        return pattern_id.replace('_', ' ').replace('-', ' ').title()

    def generate_controller_code(self):
        if not self.controller_folder:
            return
        os.makedirs(self.controller_folder, exist_ok=True)

        segments = self.led_config.get('led_segments', [])
        header_path = os.path.join(self.controller_folder, 'generated_led_config.h')
        source_path = os.path.join(self.controller_folder, 'generated_led_config.cpp')

        header_content = '''#pragma once

#include "led_string.h"

extern const uint32_t num_strings;
extern led_string_t led_strings[];
'''

        def quote(text):
            return json.dumps(text)

        string_groups = {}
        for index, segment in enumerate(segments):
            group_key = self._string_group_key(segment, index)
            if group_key not in string_groups:
                string_groups[group_key] = {
                    'name': self._string_name(segment, index),
                    'channel': self._string_channel(segment, index),
                    'segments': [],
                }
            string_groups[group_key]['segments'].append(segment)

        num_strings = len(string_groups)

        segment_blocks = []
        string_entries = []

        for index, string_group in enumerate(string_groups.values()):
            var_name = self._segment_variable_name(index, string_group['name'])
            segment_entries = []
            string_offset = 0
            for segment in string_group['segments']:
                segment_name = segment.get('name', f'segment_{index}')
                segment_entries.append(
                    f"    {{ .name = {quote(segment_name)}, .num_leds = {segment['num_leds']}, .string_offset = {string_offset} }},"
                )
                string_offset += segment['num_leds']

            segment_blocks.append(
                f"static led_segment_t {var_name}[] = {{\n" +
                "\n".join(segment_entries) +
                "\n};"
            )
            string_entries.append(
                "    {\n"
                f"        .name = {quote(string_group['name'])},\n"
                f"        .num_leds = leds_in_string({var_name}),\n"
                f"        .num_segments = segments_in_string({var_name}),\n"
                f"        .segments = {var_name},\n"
                f"        .channel = {string_group['channel']},\n"
                "        .single_color = CRGB::Red,\n"
                "        .color_ordering = WS2811_GRB,\n"
                "        .palette_index = 0,\n"
                "        .update_period_ms = 3000,\n"
                "        .brightness = 255,\n"
                "    },"
            )

        source_content = f'''#include "generated_led_config.h"

const uint32_t num_strings = {num_strings};

{os.linesep.join(segment_blocks)}

led_string_t led_strings[] = {{
{os.linesep.join(string_entries)}
}};
'''

        with open(header_path, 'w', encoding='utf-8') as header_fp:
            header_fp.write(header_content)

        with open(source_path, 'w', encoding='utf-8') as source_fp:
            source_fp.write(source_content)

    def generate_pattern_file_code(self, pattern_ids):
        if not self.controller_folder:
            return

        led_pattern_header_path = os.path.join(self.controller_folder, 'generated_led_patterns.h')
        led_pattern_source_path = os.path.join(self.controller_folder, 'generated_led_patterns.cpp')

        pattern_file_defs = []
        load_calls = []
        pattern_functions = []
        pattern_entries = []

        for pattern_id in pattern_ids:
            symbol_name = self._pattern_symbol_name(pattern_id)
            function_name = self._pattern_function_name(pattern_id)
            display_name = self._pattern_display_name(pattern_id)

            pattern_file_defs.append(
                "pattern_file_t {} = {{\n"
                "    .filepath = \"{}.bin\",\n"
                "}};".format(symbol_name, pattern_id)
            )
            load_calls.append(f"    PATTERN_FILE_LOAD({symbol_name});")
            pattern_functions.append(
                "void {}(uint32_t time_ms, uint32_t period_ms, const CRGBPalette16 *palette, CRGB single_color, uint32_t string_index, uint32_t segment_index, uint32_t num_leds, CRGB *leds)\n"
                "{{\n"
                "    pattern_file(&{}, time_ms, string_index, segment_index, num_leds, leds);\n"
                "}}".format(function_name, symbol_name)
            )
            pattern_entries.append(
                "    {{\n"
                "        .name = \"{}\",\n"
                "        .desc = \"{}\",\n"
                "        .update = {},\n"
                "    }},".format(display_name, display_name, function_name)
            )

        led_pattern_header = '''#pragma once

#include "led_pattern.h"

extern led_pattern_t led_patterns[];
void generated_led_patterns_load();
extern const uint32_t generated_num_led_patterns;
'''

        led_pattern_source = '''#include "generated_led_patterns.h"

{pattern_file_defs}

    {pattern_functions}

led_pattern_t led_patterns[] = {{
{pattern_entries}
}};

void generated_led_patterns_load()
{{
{load_calls}
}}

const uint32_t generated_num_led_patterns = {pattern_count};
'''.format(
            pattern_functions=os.linesep.join(pattern_functions),
            pattern_file_defs=os.linesep.join(pattern_file_defs),
            pattern_entries=os.linesep.join(pattern_entries),
            load_calls=os.linesep.join(load_calls),
            pattern_count=len(pattern_ids),
        )

        with open(led_pattern_header_path, 'w', encoding='utf-8') as header_fp:
            header_fp.write(led_pattern_header)

        with open(led_pattern_source_path, 'w', encoding='utf-8') as source_fp:
            source_fp.write(led_pattern_source)

    def pattern_file_path(self, pattern_id):
        return os.path.join(self.folder, str(pattern_id) + '.bin')

    async def write_uint16(self, afp, value):
        if not 0 <= value <= 65535:  # uint16 range
            raise ValueError("Value must be within the uint16 range (0-65535)")
        # '<H' specifies little-endian unsigned short (2 bytes)
        # '>H' would be big-endian
        packed_data = struct.pack('<H', value)
        await afp.write(packed_data)

    async def write_uint8(self, afp, value):
        if not 0 <= value <= 255:  # uint8 range
            raise ValueError("Value must be within the uint8 range (0-255)")
        # '<B' specifies little-endian unsigned char (1 bytes)
        # '>B' would be big-endian
        packed_data = struct.pack('<B', value)
        await afp.write(packed_data)
            
    async def generate_file_for_pattern(self, pattern, pattern_id, max_pattern_duration):
        delta = 1.0 / self.animation_rate
        num_animation_steps = int(max_pattern_duration * self.animation_rate)
        pattern_path = self.pattern_file_path(pattern_id)
        os.makedirs(self.folder, exist_ok=True)

        print("Generating code pattern %s of type %s" %
              (pattern_id, type(pattern).__name__))

        num_pixels = 0
        for segment in pattern.segments:
            num_pixels += len(segment.colors)
    
        pixel_data = []
        for animation_index in range(num_animation_steps):
            await pattern.animate(delta)
            for segment in pattern.segments:
                for color in segment.colors:
                    pixel_data.extend([color[0], color[1], color[2]])

        # Write file
        async with async_open(pattern_path, 'wb') as afp:
            # Write header
            await self.write_uint16(afp, int("0xabcd", 16))
            await self.write_uint8(afp, WS2811_RGB)
            await self.write_uint16(afp, num_pixels)
            await self.write_uint16(afp, num_animation_steps)
            await self.write_uint16(afp, max_pattern_duration)
            # Write data
            await afp.write((''.join(chr(i) for i in pixel_data)).encode('charmap'))

    async def generate(self, patterns, max_pattern_duration):
        for pattern_id, pattern in patterns.items():
            await self.generate_file_for_pattern(pattern, pattern_id, max_pattern_duration)

    def generate_controller_code_files(self, pattern_ids):
        self.generate_controller_code()
        self.generate_pattern_file_code(pattern_ids)


async def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--led_config", type=argparse.FileType('r'),
                        default="led_config_rad.json", help="LED config file")
    parser.add_argument("-a", "--animation_rate", type=int,
                        default=20, help="The target animation rate in Hz")
    parser.add_argument("-f", "--force_update", action='store_true',
                        help="Forces update of all cached patterns. Otherwise will only update missing or incomplete patterns.")
    parser.add_argument("-m", "--max_pattern_file_duration", type=int, default=60,
                        help="The maximum duration a pattern file is generated for")
    parser.add_argument("-c", "--folder", type=str,
                        default=os.path.join(ROOT, "patterns"),
                        help="The folder to output cached pattern binary files")
    parser.add_argument("--controller_folder", type=str,
                        default=os.path.join(ROOT, "lib", "LED"),
                        help="The folder to output generated controller code files")
    parser.add_argument("--pattern_config", type=str,
                        default="DEFAULT_CONFIG.rotation",
                        help="Pattern config path to generate (supports dotted paths, e.g. DEFAULT_CONFIG.rotation)")
    args = parser.parse_args()
    led_config = json.load(args.led_config)

    config_name = args.pattern_config or "DEFAULT_CONFIG.rotation"
    selected_config = resolve_pattern_config_path(pattern_config, config_name)
    pattern_set = normalize_pattern_set(selected_config, config_name)

    os.makedirs(args.folder, exist_ok=True)

    generator = PatternGenerator(pattern_set,
                                     led_config, args.animation_rate, 
                                     args.folder,
                                     controller_folder=args.controller_folder)

    # Initialize all patterns
    patterns = {}
    for d in pattern_set:
        for pattern_id, (cls, params) in d.items():
            pattern = cls()
            for key in params:
                setattr(pattern.params, key, params[key])
            try:
                pattern.prepareSegments(led_config)
                pattern.initialize()
            except Exception as err:
                print(f"Skipping pattern {pattern_id}: {type(err).__name__}: {err}")
                continue
            patterns[pattern_id] = pattern

    if not patterns:
        raise RuntimeError("No patterns could be initialized for the given LED config")

    generator.generate_controller_code_files(sorted(patterns.keys()))
    await generator.generate(patterns, args.max_pattern_file_duration)


if __name__ == "__main__":
    asyncio.run(main())