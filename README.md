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

Tracking modes:

```max
tracking_mode smart
tracking_mode pan
tracking_mode off
```

- `smart` is the default. It tries direct and pan+180/tilt-flipped candidates, rejects candidates outside the configured pan/tilt ranges, then chooses the smallest move from the previous output.
- `pan` keeps the legacy pan-only shortest-path behavior. It can still choose an out-of-range equivalent pan and then clip, so use it only if you explicitly want that old behavior.
- `off` disables tracking and outputs the direct pan/tilt solution. This avoids history-induced wrong turns, but it can spin through atan2 wrap points.

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

You can also derive offsets from a known target and a DMX value instead of hand-tuning degrees:

```max
calibrate_pan 0. 0. 2.55 32768
calibrate_tilt 0. 0. 2.55 10923
```

With two byte arguments, the current `byte_order` is used:

```max
calibrate_tilt 0. 0. 2.55 42 171
```

### `bbb.dmx.movertrack` API quick reference

Constructor arguments:

```max
[bbb.dmx.movertrack fixture_x fixture_y fixture_z]
```

Attributes:

- `@fixture_x`, `@fixture_y`, `@fixture_z` — fixture world position.
- `@pan_range`, `@tilt_range` — addressable movement ranges in degrees. Values map to `-range/2 ... +range/2`.
- `@rot rx ry rz` — fixture-local to world rotation in degrees. Rotation order is `Rz * Ry * Rx`.
- `@pan_offset`, `@tilt_offset` — degree offsets applied after raw angle calculation and before inversion.
- `@pan_invert`, `@tilt_invert` — invert the calibrated pan/tilt angle.
- `@byte_order coarsefine|finecoarse` — byte order inside each 16-bit pan/tilt value.
- `@tracking_mode smart|pan|off` — current tracking solver. Default is `smart`.
- `@shortest_pan 1|0` — compatibility alias: `1` selects `tracking_mode smart`, `0` selects `tracking_mode off`. Do not read it as the old pan-only solver.

Messages:

```max
target x y z      // same as a plain x y z list
pos x y z         // update fixture position
range pan tilt    // update ranges
calibrate_pan target_x target_y target_z pan_u16
calibrate_tilt target_x target_y target_z tilt_u16
calibrate_tilt target_x target_y target_z tilt_byte_1 tilt_byte_2
reset             // clears tracking history only
bang              // recomputes the last target, or outputs neutral center before any target
```

`reset` does not reset attributes or fixture position. It only clears the previous pan/tilt tracking state.

## `bbb.dmx.fixturemap`

```max
[bbb.dmx.fixturemap @patch patches/example.json @universe 1 @autobang 1]
```

The left outlet outputs a 512-integer list: DMX channel 1 first, channel 512 last. The right outlet outputs status/error messages such as load failures and `dump` status. Fixture profiles live in `fixtures/`; show patch files live in `patches/`. Profile paths inside patch JSON are resolved relative to the patch file.

Attributes:

- `@patch` — patch JSON path. Loaded on object initialization.
- `@universe` — selected universe, starting at `1`.
- `@autobang` — if non-zero, successful updates immediately output the full 512-channel universe. Default is `1`.

Load / inspect messages:

```max
read patches/example.json
reload
dump
clear
reset
bang
```

Parameter messages:

```max
set spot_01 dimmer 255
set spot_01 pan 32768
set spot_01 pan_tilt 32768 32768
nset spot_01 dimmer 1.0
ptbytes spot_01 127 255 127 255
```

Raw channel messages for testing or emergency overrides:

```max
channel 512 255
channels 1 255 2 128 3 0
```

Movertrack integration is intentionally byte-tuple based for now:

```max
[bbb.dmx.movertrack ...]
|
[prepend ptbytes spot_01]
|
[bbb.dmx.fixturemap @patch patches/example.json]
```

`ptbytes` accepts `pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2` and converts them through the target fixture profile's pan/tilt byte-order metadata.
