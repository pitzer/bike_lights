import argparse
import json
import asyncio
import os
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


class PatternGenerator:
    def __init__(self, pattern_config, led_config, animation_rate, folder):
        self.patterns = {}
        self.pattern_config = pattern_config
        self.led_config = led_config
        self.animation_rate = animation_rate
        self.folder = folder

    def patterns_for_caching(self):
        for d in self.pattern_config:
            for pattern_id, _ in d.items():
                yield pattern_id

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
        for pattern_id in self.patterns_for_caching():
            pattern = patterns[pattern_id]
            await self.generate_file_for_pattern(pattern, pattern_id, max_pattern_duration)


async def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--led_config", type=argparse.FileType('r'),
                        default="led_config_rad.json", help="LED config file")
    parser.add_argument("-a", "--animation_rate", type=int,
                        default=20, help="The target animation rate in Hz")
    parser.add_argument("-f", "--force_update", action='store_true',
                        help="Forces update of all cached patterns. Otherwise will only update missing or incomplete patterns.")
    parser.add_argument("-m", "--max_cached_pattern_duration", type=int, default=60,
                        help="The maximum duration a pattern is cached for")
    parser.add_argument("-c", "--folder", type=str,
                        default=os.path.join(ROOT, "patterns"),
                        help="The folder to output code files")
    args = parser.parse_args()
    led_config = json.load(args.led_config)

    os.makedirs(args.folder, exist_ok=True)

    generator = PatternGenerator(pattern_config.DEFAULT_CONFIG,
                                     led_config, args.animation_rate, 
                                     args.folder)

    # Initialize all patterns
    patterns = {}
    for d in pattern_config.DEFAULT_CONFIG:
        for pattern_id, (cls, params) in d.items():
            pattern = cls()
            for key in params:
                setattr(pattern.params, key, params[key])
            pattern.prepareSegments(led_config)
            pattern.initialize()
            patterns[pattern_id] = pattern

    await generator.generate(patterns, args.max_cached_pattern_duration)


if __name__ == "__main__":
    asyncio.run(main())