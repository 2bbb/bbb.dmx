# bbb.dmx

DMX utility external object suite for Max/MSP.

This repository deliberately does **not** implement DMX network output. Art-Net output belongs in `bbb.artnet`. `bbb.dmx.*` objects produce, inspect, transform, and guard DMX frame data inside Max.

## Current objects

- `bbb.dmx.movertrack` — converts a 3D target position into 16-bit DMX pan/tilt bytes for a moving light.
- `bbb.dmx.fixturemap` — maps fixture parameters into a selected 512-channel DMX universe list.
- `bbb.dmx.monitor` — stores and reports multi-universe DMX frames and changed channels.
- `bbb.dmx.merge` — merges named layers across multiple universes with `priority`, `htp`, or `ltp` modes.
- `bbb.dmx.fade` — fades incoming multi-universe frames over time.
- `bbb.dmx.generator` — generates test frames such as blackout, full, range, ramp, and chase.
- `bbb.dmx.safety` — clamps, slews, freezes, blackouts, and deadman-protects multi-universe streams.
- `bbb.dmx.record` — records and plays back multi-universe frame snapshots.
- `bbb.dmx.patchcheck` — validates fixture patch/profile JSON.
- `bbb.dmx.fixtureinfo` — inspects fixture patch/profile metadata.
- `bbb.dmx.matrixmap` — samples `jit.matrix` color data into fixture parameters and outputs multi-universe DMX frames.
- `bbb.dmx.curve` — applies gamma/invert/threshold/point curves to channel ranges across one or more universes.
- `bbb.dmx.mask` — mutes, holds, allows, or forces channel ranges across one or more universes.
- `bbb.dmx.palette` — applies named normalized parameter palettes to fixture patterns.
- `bbb.dmx.scene` — recalls deterministic fixture-pattern scenes into multi-universe frames.
- `bbb.dmx.fixtureview` — decodes current DMX frame bytes through fixture patch/profile metadata.

## Build

```sh
git submodule update --init --recursive
cmake -B build -DBBB_DMX_BUILD_EXTERNALS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

macOS builds universal `.mxo` externals (`x86_64` + `arm64`). Windows builds `.mxe64` via Visual Studio 2022.

## Multi-universe convention

Most utility objects use this message shape:

```max
universe <id> <512 byte values>
```

- `id` is 1-based and sanitized to at least `1`.
- DMX addresses are 1-based: `1..512`.
- Byte values are clamped to `0..255`.
- Bare `list` input, where supported, means the object's default `@universe`.
- Multi-universe objects output the same `universe <id> <512 byte values>` format, making them chainable before a sender such as `bbb.artnet`.

This is not cosmetic. A 512-value bare list is ambiguous once a show uses more than one universe. Use explicit `universe` messages at object boundaries unless you are intentionally staying in a one-universe patch.

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

The left outlet outputs a 512-integer list for the selected universe: DMX channel 1 first, channel 512 last. The right outlet outputs status/error messages such as load failures and `dump` status. Fixture profiles live in `fixtures/`; show patch files live in `patches/`. Profile paths inside patch JSON are resolved relative to the patch file.

`fixturemap` can load patches containing multiple universes, but each object instance outputs one selected universe. If you need a fully explicit multi-universe stream, run one `fixturemap` per universe or pass its output through `prepend universe <id>` before the rest of the chain.

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

## `bbb.dmx.matrixmap`

```max
[bbb.dmx.matrixmap @patch patches/rgb-grid.example.json @map maps/rgb-grid.example.json @universe_mode all]
```

`matrixmap` reads a `char` or `float32` `jit.matrix`, samples colors at normalized positions, writes normalized values into fixture parameters (`u8`, `u16`, or `u24`), and outputs explicit multi-universe frames:

```text
universe 1 <512 values>
universe 2 <512 values>
```

It does not send Art-Net. Treat it as a fixture-aware matrix-to-DMX translator before `merge` / `fade` / `safety` / sender.

Messages:

```max
readpatch patches/rgb-grid.example.json
readmap maps/rgb-grid.example.json
jit_matrix mymatrix
reload
dump
bang
```

Important attributes:

- `@universe_mode all|selected|changed` — output all patched universes, one selected universe, or only universes whose full frame changed.
- `@universe` — selected universe when `@universe_mode selected`.
- `@plane_order rgba|argb|bgra|gray` — input plane layout.
- `@gamma`, `@brightness` — color correction.
- `@invert_x`, `@invert_y` — mirror normalized sampling coordinates.
- `@autobang` — output immediately after receiving `jit_matrix`.

Matrix map example:

```json
{
  "schema": "bbb.dmx.matrixmap.v1",
  "grid": {
    "fixture_pattern": "pixel_%03d",
    "start_index": 1,
    "cols": 2,
    "rows": 2,
    "mode": "average",
    "params": {
      "red": "r",
      "green": "g",
      "blue": "b"
    }
  }
}
```

Supported color sources are `r`, `g`, `b`, `a`, `luma`, `maxrgb`, and `constant:N`. `char` matrices are normalized with `byte / 255`; `float32` matrices are clamped directly to `0.0..1.0`. Constants in `0.0..1.0` are normalized, while values greater than `1.0` keep legacy byte-constant behavior. The package includes a minimal multi-universe RGB example plus higher-resolution RGB fixture profiles:

- `fixtures/generic.rgb.3ch.json`
- `fixtures/generic.rgb.16bit.json`
- `fixtures/generic.rgb.24bit.json`
- `patches/rgb-grid.example.json`
- `maps/rgb-grid.example.json`

## Frame utilities

### `bbb.dmx.monitor`

```max
[bbb.dmx.monitor @universe 1 @changed_only 0]
```

Input:

```max
universe 1 <512 values>
channel 1 42 255
channels 1 1 255 2 128
bang
bangall
dump
clear
```

Output is either full `universe <id> ...` frames or `changed <id> address value ...` when `@changed_only 1`.

### `bbb.dmx.merge`

```max
[bbb.dmx.merge @mode priority]
```

Messages:

```max
universe layer_a 1 <512 values>
layer layer_b 2 <512 values>
priority layer_a 10
channel layer_a 1 42 255
channels layer_b 2 1 255 2 128
clear layer_a
clear all
bangall
```

Modes:

- `priority` — highest layer priority wins per channel.
- `htp` — highest byte value wins per channel.
- `ltp` — most recently updated layer wins per channel.

### `bbb.dmx.fade`

```max
[bbb.dmx.fade @time_ms 1000 @fps 30]
```

Messages:

```max
universe 1 1000 <512 target values>
channel 1 42 255 500
channels 1 750 1 255 2 128
stop
clear
bangall
```

The object outputs interpolated `universe <id> ...` frames from a main-thread timer.

### `bbb.dmx.record`

```max
[bbb.dmx.record @fps 30 @loop 0]
```

Messages:

```max
record 1
universe 1 <512 values>
universe 2 <512 values>
record 0
play 1
play 0
frame 0
bangall
write /tmp/show.dmxrec
read /tmp/show.dmxrec
clear
```

Recording stores snapshots of the full known frame set, so multiple universes are preserved per recorded frame.

## Fixture semantic utilities

### `bbb.dmx.palette`

```max
[bbb.dmx.palette @patch patches/rgb-grid.example.json @palette palettes/example.json]
```

`palette` applies a named set of normalized fixture parameters to the current fixture state and outputs explicit `universe <id> <512 values>` frames for all universes used by the patch. Fixture ids can be filtered with wildcard patterns:

```max
apply warm_white
apply deep_blue pixel_*
```

Palette JSON:

```json
{
  "schema": "bbb.dmx.palette.v1",
  "palettes": {
    "warm_white": { "red": 1.0, "green": 0.72, "blue": 0.42 },
    "deep_blue": { "red": 0.0, "green": 0.05, "blue": 1.0 }
  }
}
```

This is intentionally normalized (`0.0..1.0`) so the same palette can target `u8`, `u16`, and `u24` fixture parameters. Unknown parameters on a fixture are ignored, not fatal. That makes broad palettes usable across mixed rigs, but it also means bad parameter names can silently do nothing; verify with `bbb.dmx.fixtureview` or `bbb.dmx.fixtureinfo`.

### `bbb.dmx.scene`

```max
[bbb.dmx.scene @patch patches/rgb-grid.example.json @scene scenes/example.json]
```

`scene` recalls a full deterministic look. Each recall resets patched fixture channels to profile defaults, applies fixture-pattern rules in JSON order, then outputs all patched universes.

```max
apply blackout
apply blue_grid
```

Scene JSON:

```json
{
  "schema": "bbb.dmx.scene.v1",
  "scenes": {
    "blue_grid": {
      "pixel_*": { "red": 0.0, "green": 0.05, "blue": 1.0, "dimmer": 1.0 }
    }
  }
}
```

Use `palette` for additive color changes on the current state. Use `scene` for repeatable recalls. Confusing those two is how you get show states that depend on whatever random frame happened before.

### `bbb.dmx.fixtureview`

```max
[bbb.dmx.fixtureview @patch patches/rgb-grid.example.json]
```

Feed it the same explicit frame stream used by the rest of the package:

```max
universe 1 <512 values>
param pixel_001 red
listparams pixel_001
fixture pixel_001
```

Typical output:

```text
param pixel_001 red type u8 universe 1 addresses 1 bytes 255 raw 255 normalized 1.0
```

This object is a diagnostic decoder, not a renderer and not a sender. It exists because debugging raw channel 237 at 2am is a waste of your life when the patch already knows that channel is `pixel_079 blue`.

## Channel transform utilities

### `bbb.dmx.curve`

```max
[bbb.dmx.curve @config curves/example.json]
```

Input and output use explicit full frames:

```max
universe 1 <512 values>
channel 1 42 255
channels 1 1 255 2 128
bangall
```

Rules are channel-range based, not fixture-semantic. `universe: 0` means all universes. `start` and `count` are 1-based DMX channel ranges. Supported curves are `linear`, `gamma`, `invert`, `threshold`, and point interpolation:

```json
{
  "schema": "bbb.dmx.curve.v1",
  "rules": [
    { "universe": 0, "start": 1, "count": 512, "curve": "gamma", "gamma": 2.2 },
    { "universe": 2, "start": 1, "count": 64, "points": [[0.0, 0.0], [0.2, 0.05], [1.0, 1.0]] }
  ]
}
```

Quick rule message:

```max
gamma 0 1 512 2.2
```

### `bbb.dmx.mask`

```max
[bbb.dmx.mask @config masks/example.json]
```

`mask` is for hard show-control constraints before the sender: blackout a range, freeze a range, whitelist only specific channels, or force a value. It is deliberately lower-level than fixture patches because it must still work when profile data is wrong.

Actions:

- `mute` — force matching channels to 0.
- `hold` — keep the previous output value for matching channels.
- `allow` — if any allow rule exists, channels not matched by an allow rule output 0.
- `force` — force matching channels to `value`.

Example config:

```json
{
  "schema": "bbb.dmx.mask.v1",
  "rules": [
    { "universe": 1, "start": 1, "count": 16, "action": "hold" },
    { "universe": 2, "start": 100, "count": 12, "action": "mute" },
    { "universe": 0, "start": 512, "count": 1, "action": "force", "value": 0 }
  ]
}
```

Quick rule messages:

```max
mute 1 100 12
hold 1 1 16
allow 2 1 128
force 0 512 1 0
```

## Test, safety, and inspection utilities

### `bbb.dmx.generator`

```max
[bbb.dmx.generator]
```

Messages:

```max
blackout 1
full 2
all 1 64
range 1 1 16 255
ramp 1 1 512 0 255
chase 1 42 255
```

### `bbb.dmx.safety`

```max
[bbb.dmx.safety @max_value 255 @max_delta 255 @timeout_ms 0 @blackout 0 @freeze 0]
```

Messages:

```max
universe 1 <512 values>
channel 1 42 255
channels 1 1 255 2 128
blackout 1
freeze 1
bangall
```

- `@max_value` clamps every output byte.
- `@max_delta` limits per-update channel jumps.
- `@timeout_ms` blackouts if no input arrives within the configured interval.
- `@blackout` forces zero output.
- `@freeze` holds the last safe frame.

### `bbb.dmx.patchcheck`

```max
[bbb.dmx.patchcheck @patch patches/example.json]
```

Messages:

```max
read patches/example.json
bang
```

Output:

```text
ok fixtures <count> universes <id...>
error <message>
```

### `bbb.dmx.fixtureinfo`

```max
[bbb.dmx.fixtureinfo @patch patches/example.json]
```

Messages:

```max
bang
listfixtures
fixture spot_01
listparams spot_01
param spot_01 pan
dump
```

Typical output selectors are `summary`, `fixture`, `param`, and `error`.
