# SPEC.fixturemap.md — fixture data and 512-channel universe mapping

## 1. Goal

Add a common fixture-mapping layer for the `bbb.dmx.*` suite.

The layer takes:

- one DMX universe buffer (`512` channels, 1-based DMX channel numbers)
- fixture profile data (what channels a fixture mode exposes)
- fixture patch data (which profile/mode is placed at which universe/address)
- runtime parameter updates (`fixture.parameter value`)

It outputs:

- a fully resolved `512`-integer DMX universe list, each value clamped to `0..255`

This is intentionally separate from `bbb.dmx.movertrack`. `movertrack` computes values. The mapper decides where those values land in a universe.

---

## 2. Non-goals

Do **not** make manufacturer fixture files the internal source of truth.

GDTF, OFL, vendor PDFs, and hand-written Max patches all disagree in detail. Importers can be added later, but the runtime must use one small normalized schema that we control.

Do **not** mix fixture profile and stage patch into one flat object.

A fixture profile describes a model/mode. A fixture instance describes a patched unit in a show.

---

## 3. Core model

### 3.1 Universe

A universe is exactly `512` byte channels.

Rules:

- DMX addresses are user-facing and `1..512`.
- Internal array indices are `0..511`.
- Address overflow is an error by default.
- Optional clipping mode may be added later, but silent overflow must never be default behavior.

### 3.2 Fixture profile

A profile describes reusable fixture capabilities.

A profile contains:

- `key`: stable machine key, e.g. `generic.mover.16bit`
- `manufacturer`: display string
- `model`: display string
- `modes`: named DMX layouts

A mode contains:

- `key`: mode key, e.g. `basic16`
- `footprint`: DMX channel count
- `channels`: relative channel definitions, using 1-based offsets within the fixture footprint
- `parameters`: semantic parameter definitions mapped onto one or more channels

### 3.3 Fixture instance / patch

A patch instance contains:

- `id`: stable show-local fixture id, e.g. `spot_01`
- `profile`: profile key
- `mode`: mode key
- `universe`: integer universe id; for the first mapper object this is normally `1`
- `address`: DMX start address, `1..512`
- optional spatial/calibration data used by other `bbb.dmx.*` objects

The mapper should allow multiple universes in the patch file, but a single object instance may select one universe to output.

---

## 4. Canonical file format

Use Max `dict`-compatible JSON as the canonical format.

Reason:

- Max already understands `dict` and JSON well enough.
- It is readable and diffable.
- It works cross-platform without extra dependencies.
- It can be generated from future importers.

Recommended package locations:

```text
fixtures/        # reusable profile JSON files
patches/         # show patch JSON files
```

The release package should include these folders once the object exists.

---

## 5. Profile schema

### 5.1 Minimal profile example

```json
{
  "schema": "bbb.dmx.fixture.profile.v1",
  "key": "generic.mover.16bit",
  "manufacturer": "Generic",
  "model": "16-bit Pan/Tilt Mover",
  "modes": {
    "basic16": {
      "label": "Basic 16-bit",
      "footprint": 8,
      "channels": [
        { "offset": 1, "key": "pan.coarse",  "default": 128 },
        { "offset": 2, "key": "pan.fine",    "default": 0 },
        { "offset": 3, "key": "tilt.coarse", "default": 128 },
        { "offset": 4, "key": "tilt.fine",   "default": 0 },
        { "offset": 5, "key": "dimmer",      "default": 0 },
        { "offset": 6, "key": "shutter",     "default": 0 },
        { "offset": 7, "key": "color",       "default": 0 },
        { "offset": 8, "key": "gobo",        "default": 0 }
      ],
      "parameters": {
        "pan": {
          "type": "u16",
          "channels": ["pan.coarse", "pan.fine"],
          "byte_order": "coarsefine",
          "range_degrees": 540,
          "default": 32768
        },
        "tilt": {
          "type": "u16",
          "channels": ["tilt.coarse", "tilt.fine"],
          "byte_order": "coarsefine",
          "range_degrees": 270,
          "default": 32768
        },
        "dimmer": {
          "type": "u8",
          "channel": "dimmer",
          "default": 0
        },
        "shutter": {
          "type": "u8",
          "channel": "shutter",
          "default": 0
        }
      }
    }
  }
}
```

### 5.2 Channel object

Required fields:

| Field | Type | Meaning |
|---|---|---|
| `offset` | int | Relative DMX channel, `1..footprint` |
| `key` | string | Stable channel key within the mode |

Optional fields:

| Field | Type | Default | Meaning |
|---|---|---:|---|
| `default` | int | `0` | Startup byte value, clamped `0..255` |
| `label` | string | `key` | Human-readable label |
| `hold` | bool | `false` | If true, preserve previous value on reset |

### 5.3 Parameter object

Supported `type` values for v1:

| Type | Runtime value | Channel write |
|---|---|---|
| `u8` | `0..255` or normalized `0..1` | one byte |
| `u16` | `0..65535` or normalized `0..1` | two bytes |
| `enum` | symbol or index | one byte from range table |

For v1, `float physical unit -> DMX` conversion should only be implemented where it is essential:

- `pan` / `tilt` may expose `range_degrees`
- generic channels should use raw byte or normalized values first

Trying to model every fixture's physical units immediately is fake precision. It will create bugs faster than value.

---

## 6. Patch schema

### 6.1 Minimal patch example

```json
{
  "schema": "bbb.dmx.patch.v1",
  "profiles": [
    "fixtures/generic.mover.16bit.json"
  ],
  "fixtures": [
    {
      "id": "spot_01",
      "profile": "generic.mover.16bit",
      "mode": "basic16",
      "universe": 1,
      "address": 1,
      "position": [0.0, 0.0, 3.0],
      "rotation": [0.0, 0.0, 0.0],
      "calibration": {
        "pan_offset": 0.0,
        "tilt_offset": 0.0,
        "pan_invert": false,
        "tilt_invert": false
      }
    },
    {
      "id": "spot_02",
      "profile": "generic.mover.16bit",
      "mode": "basic16",
      "universe": 1,
      "address": 17,
      "position": [2.0, 0.0, 3.0]
    }
  ]
}
```

### 6.2 Patch validation

Loading a patch must validate:

- every fixture id is unique
- every referenced profile exists
- every referenced mode exists
- `1 <= address <= 512`
- `address + footprint - 1 <= 512` for the selected output universe
- no channel overlaps unless explicitly allowed

Default overlap policy: `error`.

Optional future policies:

- `last_wins`
- `highest_takes_precedence`
- `merge_htp_for_dimmer_only`

Do not implement these policies first. Start strict.

---

## 7. Runtime API proposal

### 7.1 New object

```max
[bbb.dmx.fixturemap @patch patches/show.json @universe 1]
```

Output:

```text
512 integers, channel 1 first, channel 512 last
```

### 7.2 Messages

#### Load / inspect

```max
read patches/show.json
reload
dump
clear
reset
```

#### Set fixture parameter

```max
set spot_01 dimmer 255
set spot_01 pan 32768
set spot_01 tilt 32768
set spot_01 pan_tilt 32768 32768
```

#### Normalized input

```max
nset spot_01 dimmer 1.0
nset spot_01 pan 0.5
nset spot_01 tilt 0.5
```

`nset` maps `0.0..1.0` onto the parameter's DMX range.

#### Raw channel override

```max
channel 1 255
channels 1 255 2 128 3 0
```

Raw override is useful for testing, but it should be visibly separate from fixture parameters. Otherwise patches become unreadable.

#### Movertrack integration

`bbb.dmx.movertrack` currently outputs:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

The mapper should support either:

```max
ptbytes spot_01 pan1 pan2 tilt1 tilt2
```

or, preferably after a later movertrack update:

```max
set16 spot_01 pan pan_u16 tilt tilt_u16
```

The better long-term API is 16-bit semantic values, not byte tuples. Byte tuples are transport detail.

---

## 8. Output behavior

Attributes:

```max
@autobang 1
@dirty_only 0
@include_selector 0
```

Recommended v1 behavior:

- Every successful value update recomputes the internal universe buffer.
- If `@autobang 1`, immediately output the full 512-byte list.
- `bang` always outputs the current full universe.
- `reset` restores profile defaults and outputs if `@autobang 1`.

Later optimization:

- `@dirty_only 1` outputs changed `(channel value)` pairs from a second outlet.
- Full universe list remains available because many Max patches prefer a simple list.

---

## 9. Error policy

This object must be strict and loud during load, quiet during high-rate runtime updates.

Load-time errors:

- bad JSON/dict
- missing profile
- bad mode
- footprint overflow
- overlap

Runtime warnings should be once-per-condition:

- unknown fixture id
- unknown parameter
- out-of-range value clamped
- non-finite number ignored

Invalid runtime updates must not corrupt the universe buffer.

---

## 10. Shared C++ layer

Add these Max-independent types under `source/bbb/dmx/`:

```text
fixture_profile.hpp
fixture_patch.hpp
universe.hpp
fixture_mapper.hpp
```

Suggested core types:

```cpp
struct dmx_universe {
    std::array<std::uint8_t, 512> channels;
};

struct fixture_profile;
struct fixture_mode;
struct fixture_parameter;
struct fixture_instance;

class fixture_mapper {
public:
    bool load_profiles(...);
    bool load_patch(...);
    bool set_u8(std::string fixture_id, std::string parameter, int value);
    bool set_u16(std::string fixture_id, std::string parameter, std::uint16_t value);
    bool set_normalized(std::string fixture_id, std::string parameter, double value);
    const dmx_universe &universe(int universe_id) const;
};
```

The JSON parser should stay outside the pure mapping math if possible. Tests should construct C++ objects directly first, then test parsing second.

---

## 11. Implementation order

1. `dmx_universe` and low-level write helpers:
   - address conversion
   - bounds checks
   - u8/u16 byte-order writes
2. Profile/mode/parameter data structures.
3. Strict patch validator.
4. `fixture_mapper` C++ tests with hand-built fixtures.
5. JSON/dict loader.
6. `bbb.dmx.fixturemap` external.
7. Help patch showing:
   - load patch
   - set dimmer
   - feed movertrack pan/tilt
   - output 512-channel list

Do not start with a GUI editor. That is premature. The first useful tool is a deterministic mapper with strict validation.

---

## 12. Design decision

Canonical fixture data should be specified as two JSON/dict layers:

1. reusable fixture profiles in `fixtures/*.json`
2. show-specific fixture patches in `patches/*.json`

This is the lowest-friction path for Max users while still being strict enough to support future importers and CI tests.
