# bbb.dmx.fixtureinfo

`bbb.dmx.fixtureinfo` inspects loaded bbb.dmx patch/profile metadata without outputting DMX.
Use it when you need to debug fixture JSON, discover available fixture parameters, or build Max UI around fixture/profile metadata.

It does **not** send universe frames. For DMX output use `bbb.dmx.fixturemap`, `bbb.dmx.matrixmap`, or another generator/mapper object.

## Loading data

```max
[bbb.dmx.fixtureinfo @patch patches/example.json]
```

or:

```max
read patches/example.json
```

The object loads a patch JSON file (`bbb.dmx.patch.v2`). Fixture profiles are loaded from the patch file's `profiles` array.

Important constraints:

- Profile/mode inspection only works for profiles that have been loaded through the patch JSON.
- Relative paths are resolved through the shared bbb.dmx Max file resolver. In practice, put project data where Max can resolve it, or use an absolute path if Max path lookup is ambiguous.
- Numeric fixture IDs in imported MVR/MA3 data are normalized to strings internally. In Max, `fixture 101` and `listparams 101` target fixture id `"101"`.

## Messages

### `bang`

Outputs a patch summary.

```max
bang
```

Output:

```text
summary fixtures <count> universes <universe_id...>
```

Example:

```text
summary fixtures 2 universes 1
```

### `dump`

Outputs the summary followed by every patched fixture.

```max
dump
```

Equivalent to:

```max
bang
listfixtures
```

### `listfixtures`

Outputs one `fixture` message for every fixture instance in the loaded patch.

```max
listfixtures
```

Output:

```text
fixture <fixture_id> universe <id> address <dmx_address> profile <profile_key> mode <mode_key> footprint <channels> position <x> <y> <z>
```

Example:

```text
fixture spot_01 universe 1 address 1 profile generic.mover.16bit mode basic16 footprint 8 position 0. 0. 3.
```

### `fixture <fixture_id>`

Outputs metadata for one patched fixture instance.

```max
fixture spot_01
fixture 101
```

`fixture_id` is the patch fixture `id`, not the fixture profile key.

### `listparams <fixture_id>`

Lists parameters for the mode used by a patched fixture instance.

```max
listparams spot_01
listparams 101
```

Output selector is `param`.

```text
param <fixture_id> <parameter_key> type <u8|u16|u24|enum_u8> channels <channel_key...> byte_order <order> default <value> range_degrees <degrees>
```

Example:

```text
param spot_01 pan type u16 channels pan.coarse pan.fine byte_order coarsefine default 32768 range_degrees 540.
```

Use this form when you care about a fixture instance already placed in the patch.

### `param <fixture_id> <parameter_key>`

Outputs one parameter for a patched fixture instance.

```max
param spot_01 pan
param 101 dimmer
```

Errors if the fixture id is unknown or the parameter does not exist on that fixture's mode.

### `listparams <profile_key> <mode_key>`

Lists parameters for a loaded fixture profile/mode directly, without requiring a patched fixture id.

```max
listparams generic.mover.16bit basic16
listparams generic.rgb.16bit rgb16
```

Output:

```text
param <profile_key> <mode_key> <parameter_key> type <u8|u16|u24|enum_u8> channels <channel_key...> byte_order <order> default <value> range_degrees <degrees>
```

Example:

```text
param generic.rgb.16bit rgb16 red type u16 channels red.coarse red.fine byte_order coarsefine default 0 range_degrees 0.
```

This is the recommended form for UI builders or tooling that starts from “fixture name + mode”.

### `modeparams <profile_key> <mode_key>`

Explicit alias for profile/mode parameter listing.

```max
modeparams generic.mover.16bit basic16
```

Use `modeparams` when your patch already uses `listparams` for fixture-instance queries and you want a visually unambiguous message.

### `param <profile_key> <mode_key> <parameter_key>`

Outputs one parameter for a loaded profile/mode.

```max
param generic.mover.16bit basic16 pan
param generic.rgb.16bit rgb16 red
```

Output shape is the same as `listparams <profile_key> <mode_key>`.

## Parameter detail fields

Every parameter output includes:

| Field | Meaning |
|---|---|
| `type` | Parameter storage type: `u8`, `u16`, `u24`, or `enum_u8` |
| `channels` | Fixture channel keys used by the parameter, in write order |
| `byte_order` | Multi-byte ordering. Usually `coarsefine`; may be `finecoarse`, `coarsemidfine`, or `finemidcoarse` |
| `default` | Raw default value in the parameter's native integer range |
| `range_degrees` | Physical angular range for pan/tilt style parameters, or `0.` when not specified |

### 8-bit parameters

```text
param spot_01 dimmer type u8 channels dimmer byte_order coarsefine default 0 range_degrees 0.
```

Use normalized writes through `fixturemap nset/nsetall`, or raw 0..255 values through `set/setall` depending on the downstream object.

### 16-bit parameters

```text
param generic.rgb.16bit rgb16 red type u16 channels red.coarse red.fine byte_order coarsefine default 0 range_degrees 0.
```

Do not assume you need to address `red.coarse` manually. In bbb.dmx fixture APIs, the canonical parameter key is `red`; the mapper expands it to `red.coarse` / `red.fine` using `type` and `byte_order`.

### 24-bit parameters

```text
param generic.rgb.24bit rgb24 red type u24 channels red.coarse red.middle red.fine byte_order coarsemidfine default 0 range_degrees 0.
```

24-bit parameters are mainly for high-resolution color or custom devices. Treat values as normalized unless you explicitly need raw integer control.

### Enum parameters

`enum_u8` uses one DMX channel but includes semantic ranges in the fixture profile. `fixtureinfo` currently reports the parameter summary, not the individual enum ranges. If you need range-level UI labels, inspect the profile JSON or extend `fixtureinfo` with a range-dump message.

## Routing examples in Max

Route summary, fixture, parameter, and errors:

```max
[bbb.dmx.fixtureinfo @patch patches/example.json]
|
[route summary fixture param error]
```

List profile/mode parameters and strip the fixed prefix:

```max
[message listparams generic.rgb.16bit rgb16]
|
[bbb.dmx.fixtureinfo @patch patches/rgb-grid.example.json]
|
[route param]
|
[route generic.rgb.16bit]
|
[route rgb16]
```

After that route chain, each parameter message begins with:

```text
<parameter_key> type ...
```

## Common mistakes

### “unknown profile” for a valid fixture JSON file

`fixtureinfo` does not scan arbitrary fixture profile files by name. The profile must be referenced by the loaded patch JSON:

```json
{
  "profiles": [
    "../fixtures/generic.mover.16bit.json"
  ]
}
```

### Confusing fixture id and profile key

These are different:

```json
{
  "id": "spot_01",
  "profile": "generic.mover.16bit",
  "mode": "basic16"
}
```

Use fixture-instance queries:

```max
listparams spot_01
param spot_01 pan
```

Use profile/mode queries:

```max
listparams generic.mover.16bit basic16
modeparams generic.mover.16bit basic16
param generic.mover.16bit basic16 pan
```

### Expecting DMX output

`fixtureinfo` is an inspection object. It only outputs metadata messages. It never outputs `universe <id> <512 bytes>`.

### Assuming missing parameters are harmless

Unlike broad palette or `setall` style operations that may intentionally ignore unknown parameters per fixture, `fixtureinfo param ...` reports unknown parameters as errors. That is correct: this object is for validation/debugging, not broad best-effort writes.

## Related objects

- `bbb.dmx.fixturemap` — maps fixture parameter writes to universe frames.
- `bbb.dmx.fixtureview` — inspects fixture state after DMX data is applied.
- `bbb.dmx.patchcheck` — validates patch/profile JSON and reports aggregate status.
- `bbb.dmx.matrixmap` — maps `jit.matrix` color samples to fixture parameters.
