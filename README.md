# bbb.dmx

DMX utility external object suite for Max/MSP.

Current objects:

- `bbb.dmx.movertrack` — converts a 3D target position into 16-bit DMX pan/tilt bytes for a moving light.
- `bbb.dmx.fixturemap` — maps fixture parameters into a full 512-channel DMX universe list.

## Build

```sh
git submodule update --init --recursive
cmake -B build -DBBB_DMX_BUILD_EXTERNALS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

macOS builds a universal `.mxo` (`x86_64` + `arm64`). Windows builds `.mxe64` via Visual Studio 2022.

## `bbb.dmx.movertrack`

```max
[bbb.dmx.movertrack 0. 0. 3. @pan_range 540. @tilt_range 270. @rot 0. 0. 0.]
```

Input a target position list:

```max
0. 5. 1.5
```

Output:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

Default byte order is `coarsefine`.

Coordinate convention:

```text
+X = stage right / local right
+Y = forward / pan center
+Z = up
```

For a ceiling-hung fixture mounted upside down with pan center facing the `y = 0` side of the room/stage, use:

```max
[bbb.dmx.movertrack 0. 3.84 2.55 @rot 180. 0. 0. @tilt_invert 0 @tilt_offset -90.]
```

This separates the two corrections:

- `@rot 180. 0. 0.` describes the physical upside-down orientation.
- `@tilt_offset -90.` calibrates the fixture-specific tilt horizontal point.
- Keep `@tilt_invert 0` for this installation; `@rot` already accounts for the world up/down reversal.

## `bbb.dmx.fixturemap`

```max
[bbb.dmx.fixturemap @patch patches/example.json @universe 1]
```

Useful messages:

```max
set spot_01 dimmer 255
set spot_01 pan 32768
set spot_01 pan_tilt 32768 32768
nset spot_01 dimmer 1.0
ptbytes spot_01 127 255 127 255
bang
```

The object outputs a 512-integer list: DMX channel 1 first, channel 512 last. Fixture profiles live in `fixtures/`; show patch files live in `patches/`. Profile paths inside patch JSON are resolved relative to the patch file.
