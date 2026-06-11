# bbb.dmx JSON Format Specification

This document is the canonical interchange specification for `bbb.dmx` JSON files. It is intended for Max patches, converter tools, and external applications that want to generate or consume `bbb.dmx` fixture, patch, mapping, palette, scene, curve, mask, and assertion data.

If this document conflicts with an older README snippet, this document wins. If it conflicts with current C++ behavior, treat the conflict as a compatibility bug or an intentionally documented legacy tolerance. The C++ parser may accept some loose input for old patches; external applications should emit the stricter canonical form described here.

## 1. Global conventions

### 1.1 Encoding and JSON subset

- Files are UTF-8 JSON text.
- Top-level value is always an object.
- Every canonical file **must** include a top-level `schema` string.
- Object member order is not semantically meaningful, but examples use stable ordering for human review.
- Unknown properties are reserved. Producers should not emit them unless a future schema version defines them.

### 1.2 Versioning

`schema` is a stable identifier, not a URL:

```json
"schema": "bbb.dmx.fixture.profile.v1"
```

Current schema ids:

| File kind | `schema` value | JSON Schema |
|---|---|---|
| Fixture profile | `bbb.dmx.fixture.profile.v1` | `schemas/bbb.dmx.fixture.profile.v1.schema.json` |
| Fixture patch | `bbb.dmx.patch.v1` | `schemas/bbb.dmx.patch.v1.schema.json` |
| Matrix map | `bbb.dmx.matrixmap.v1` | `schemas/bbb.dmx.matrixmap.v1.schema.json` |
| Palette set | `bbb.dmx.palette.v1` | `schemas/bbb.dmx.palette.v1.schema.json` |
| Scene set | `bbb.dmx.scene.v1` | `schemas/bbb.dmx.scene.v1.schema.json` |
| Curve rules | `bbb.dmx.curve.v1` | `schemas/bbb.dmx.curve.v1.schema.json` |
| Mask rules | `bbb.dmx.mask.v1` | `schemas/bbb.dmx.mask.v1.schema.json` |
| Assertion rules | `bbb.dmx.assert.v1` | `schemas/bbb.dmx.assert.v1.schema.json` |

Rules for future versions:

- Additive optional fields may stay within `v1` only if older consumers can safely ignore them.
- Any semantic change to existing fields requires a new schema id, for example `bbb.dmx.patch.v2`.
- Do not silently change units, indexing, or default behavior inside an existing schema id.

### 1.3 Numeric conventions

| Concept | Convention |
|---|---|
| DMX universe id | Integer, 1-based. Some rule files use `0` to mean “all known universes”; that exception is documented per file type. |
| DMX address/channel | Integer, 1-based, `1..512`. |
| DMX byte | Integer, `0..255`. |
| Normalized parameter value | Number, canonical range `0.0..1.0`. Runtime clamps values before writing fixture parameters. Producers should emit clamped values. |
| 16-bit value | Integer, `0..65535`, split according to `byte_order`. |
| 24-bit value | Integer, `0..16777215`, split according to `byte_order`. |
| Position/rotation | 3-number arrays. Units are project-defined; rotations are degrees. |

### 1.4 Names and ids

Use stable ASCII-ish identifiers where possible:

- Fixture profile keys: `manufacturer.model.variant`, e.g. `generic.rgb.3ch`.
- Fixture ids: patch-local ids, e.g. `spot_01`, `pixel_001`.
- Mode keys: short stable names, e.g. `basic16`, `rgb`.
- Parameter keys: semantic names, e.g. `pan`, `tilt`, `dimmer`, `red`, `green`, `blue`.

The implementation stores keys as strings and does not require a regex, but external applications should avoid spaces and punctuation other than `.`, `_`, and `-`.

### 1.5 Channel byte order strings

Accepted byte-order strings:

| String | Meaning |
|---|---|
| `coarsefine` | 16-bit MSB then LSB. |
| `finecoarse` | 16-bit LSB then MSB. |
| `coarsemidfine` | 24-bit MSB, middle, LSB. |
| `finemidcoarse` | 24-bit LSB, middle, MSB. |

Current C++ also accepts aliases `coarsemiddlefine`, `finemiddlecoarse`, `msbfirst`, and `lsbfirst` in some paths. They are compatibility aliases, not canonical output.

## 2. Fixture profile: `bbb.dmx.fixture.profile.v1`

A fixture profile describes one reusable fixture model and its available DMX modes. It does **not** describe where a fixture is patched in a show.

Typical path: `fixtures/*.json`.

### 2.1 Shape

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
        { "offset": 1, "key": "pan.coarse", "default": 128 },
        { "offset": 2, "key": "pan.fine", "default": 0 }
      ],
      "parameters": {
        "pan": {
          "type": "u16",
          "channels": ["pan.coarse", "pan.fine"],
          "byte_order": "coarsefine",
          "range_degrees": 540,
          "default": 32768
        }
      }
    }
  }
}
```

### 2.2 Top-level fields

| Field | Type | Required | Meaning |
|---|---:|---:|---|
| `schema` | string | yes | Must be `bbb.dmx.fixture.profile.v1`. |
| `key` | string | yes | Stable profile identifier used by patches. |
| `manufacturer` | string | recommended | Human-readable manufacturer. Current runtime allows omission and treats it as empty. |
| `model` | string | recommended | Human-readable model. Current runtime allows omission and treats it as empty. |
| `modes` | object | yes | Map of `mode_key -> mode object`. |

### 2.3 Mode object

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `label` | string | no | `""` | Human-readable label. |
| `footprint` | integer | yes | — | Number of DMX channels consumed by this mode. Must be `1..512`. |
| `channels` | array | yes | — | Ordered channel metadata. |
| `parameters` | object | recommended | empty | Semantic parameter mapping. Required for `fixturemap`, `matrixmap`, `palette`, `scene`, and `movertrack` integration to do meaningful work. |

`channels[].offset` values are relative to the fixture start address and are 1-based. A fixture patched at address `17` with a channel offset `3` writes absolute DMX address `19`.

### 2.4 Channel object

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `offset` | integer | yes | — | 1-based offset within the mode, `1..footprint`. |
| `key` | string | yes | — | Channel key referenced by parameters. |
| `default` | integer | no | `0` | DMX byte default, `0..255`. |
| `label` | string | no | `""` | Human label. |
| `hold` | boolean | no | `false` | Reserved semantic hint. Current core loader stores it; most objects do not act on it yet. |

Do not define duplicate `offset` values or duplicate `key` values inside one mode. The current JSON loader does not fully police this, but fixture writers will behave ambiguously.

### 2.5 Parameter object

A parameter maps one semantic control value to one or more channel keys.

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `type` | string | yes | — | `u8`, `u16`, `u24`, or `enum`. |
| `channel` | string | for `u8`/`enum` unless `channels` is used | — | Single channel key. |
| `channels` | array of strings | for `u16`/`u24` | — | Multi-byte channel keys in declared `byte_order`. |
| `byte_order` | string | no | `coarsefine` | Byte order for `u16`/`u24`. |
| `default` | integer | no | `0` | Semantic default before splitting. For `u8` max is `255`; for `u16` max is `65535`; for `u24` max is `16777215`. |
| `range_degrees` | number | pan/tilt recommended | `0` | Physical movement range used by mover tracking. |

Canonical channel counts:

| `type` | Expected channels |
|---|---:|
| `u8` | 1 |
| `enum` | 1 |
| `u16` | 2 |
| `u24` | 3 |

`enum` is currently stored as an 8-bit value. Enumeration labels are not part of v1 yet.

## 3. Fixture patch: `bbb.dmx.patch.v1`

A patch describes fixture instances in a show: profile, mode, universe, address, and optional placement/calibration.

Typical path: `patches/*.json`.

### 3.1 Shape

```json
{
  "schema": "bbb.dmx.patch.v1",
  "profiles": ["../fixtures/generic.mover.16bit.json"],
  "fixtures": [
    {
      "id": "spot_01",
      "profile": "generic.mover.16bit",
      "mode": "basic16",
      "universe": 1,
      "address": 1,
      "position": [0.0, 0.0, 3.0],
      "rotation": [180.0, 0.0, 0.0],
      "calibration": {
        "pan_offset": 0.0,
        "tilt_offset": -90.0,
        "pan_invert": false,
        "tilt_invert": false
      }
    }
  ]
}
```

### 3.2 Top-level fields

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `schema` | string | yes | — | Must be `bbb.dmx.patch.v1`. |
| `profiles` | array of strings | no | empty | Profile JSON paths. Paths are resolved relative to the patch file by Max utilities. |
| `fixtures` | array | yes | — | Fixture instances. |

### 3.3 Fixture instance

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `id` | string | yes | — | Unique patch-local fixture id. |
| `profile` | string | yes | — | Must match a loaded profile `key`. |
| `mode` | string | yes | — | Must match a mode key in the profile. |
| `universe` | integer | yes | — | 1-based DMX universe id. |
| `address` | integer | yes | — | 1-based start address, `1..512`. |
| `position` | `[x,y,z]` | no | `[0,0,0]` | Fixture origin in project/world coordinates. |
| `rotation` | `[rx,ry,rz]` | no | `[0,0,0]` | Fixture rotation in degrees. Current mover math composes X/Y/Z rotations. |
| `calibration` | object | no | all zero/false | Fixture-specific mover calibration. |

`address + footprint - 1` should be `<= 512`. Current `patchcheck` reports overlap/range problems; producers should avoid generating invalid patches.

### 3.4 Calibration

| Field | Type | Default | Meaning |
|---|---:|---:|---|
| `pan_offset` | number | `0.0` | Added to computed pan angle in degrees. |
| `tilt_offset` | number | `0.0` | Added to computed tilt angle in degrees. |
| `pan_invert` | boolean | `false` | Invert pan direction. |
| `tilt_invert` | boolean | `false` | Invert tilt direction. |

Your current inverted hanging setup, for example, is represented by fixture `rotation` plus calibration; do not bake that correction into fixture profiles.

## 4. Matrix map: `bbb.dmx.matrixmap.v1`

A matrix map samples a `jit.matrix` and writes normalized color sources into fixture parameters through a patch/profile.

Typical path: `maps/*.json`.

### 4.1 Explicit fixture mapping

```json
{
  "schema": "bbb.dmx.matrixmap.v1",
  "fixtures": [
    {
      "id": "pixel_001",
      "sample": { "mode": "average", "x": 0.25, "y": 0.25, "w": 0.5, "h": 0.5 },
      "params": { "red": "r", "green": "g", "blue": "b" }
    }
  ]
}
```

### 4.2 Grid mapping

```json
{
  "schema": "bbb.dmx.matrixmap.v1",
  "grid": {
    "fixture_pattern": "pixel_%03d",
    "start_index": 1,
    "cols": 2,
    "rows": 2,
    "mode": "average",
    "params": { "red": "r", "green": "g", "blue": "b" }
  }
}
```

Top-level must contain at least one of `fixtures`, `grid`, or `grids`.

### 4.3 Fixture mapping fields

| Field | Type | Required | Meaning |
|---|---:|---:|---|
| `id` | string | yes | Fixture id from the patch. |
| `sample` | object | yes | Matrix sample region. |
| `params` | object | yes | Parameter-to-source bindings. |

### 4.4 Sample region

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `mode` | string | no | `point` for explicit fixtures, `average` for grid | `point`, `average`, `max`, or `min`. |
| `x` | number | yes | — | Normalized X sample center, clamped to `0..1`. |
| `y` | number | yes | — | Normalized Y sample center, clamped to `0..1`. |
| `w` | number | no | `0` | Normalized width for region modes, clamped to `0..1`. |
| `h` | number | no | `0` | Normalized height for region modes, clamped to `0..1`. |

### 4.5 Grid fields

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `fixture_pattern` | string | yes | — | `printf`-style pattern receiving fixture index, e.g. `pixel_%03d`. |
| `cols` | integer | yes | — | Grid columns, positive. |
| `rows` | integer | yes | — | Grid rows, positive. |
| `start_index` | integer | no | `1` | First fixture index. |
| `mode` | string | no | `average` | Sample mode for all generated cells. |
| `params` | object | yes | — | Parameter-to-source bindings. |

`grids` is an array of grid objects. `grid` is a single grid object convenience form.

### 4.6 Parameter source bindings

`params` maps fixture parameter names to either a source string or numeric constant.

| Source | Meaning |
|---|---|
| `r` | Red matrix plane. |
| `g` | Green matrix plane. |
| `b` | Blue matrix plane. |
| `a` | Alpha matrix plane. |
| `luma` | Luminance derived from RGB. |
| `maxrgb` | Max of RGB. |
| `constant:N` | Constant value. `N <= 1.0` is normalized; `N > 1.0` is legacy byte-style and converted with `N / 255`. |
| number | Constant value with the same normalization rule as above. |

`char` matrices are normalized as `byte / 255`. `float32` matrices are clamped directly to `0.0..1.0`.

## 5. Palette set: `bbb.dmx.palette.v1`

A palette set stores named normalized parameter values. Applying a palette modifies the current fixture state; it does not reset all fixture channels first.

Typical path: `palettes/*.json`.

```json
{
  "schema": "bbb.dmx.palette.v1",
  "palettes": {
    "warm_white": { "red": 1.0, "green": 0.72, "blue": 0.42 }
  }
}
```

| Field | Type | Required | Meaning |
|---|---:|---:|---|
| `schema` | string | yes | Must be `bbb.dmx.palette.v1`. |
| `palettes` | object | yes | Map of `palette_name -> parameter map`. |

A parameter map is an object whose keys are fixture parameter names and whose values are numbers. Canonical values are normalized `0.0..1.0`; runtime clamps when writing to fixture parameters. Unknown parameters are ignored per fixture, allowing broad palettes across mixed rigs. This also means spelling mistakes can silently do nothing; validate with `fixtureview` or `fixtureinfo`.

## 6. Scene set: `bbb.dmx.scene.v1`

A scene set stores deterministic recalls. Applying a scene resets patched fixture channels to profile defaults, applies matching fixture-pattern rules in JSON order, then outputs all patched universes.

Typical path: `scenes/*.json`.

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

| Field | Type | Required | Meaning |
|---|---:|---:|---|
| `schema` | string | yes | Must be `bbb.dmx.scene.v1`. |
| `scenes` | object | yes | Map of `scene_name -> fixture rule map`. |

A fixture rule map is an object whose keys are fixture id wildcard patterns and whose values are normalized parameter maps.

Pattern syntax follows the runtime wildcard matcher:

- `*` matches zero or more characters.
- `?` matches one character.
- All other characters match literally.

If multiple patterns match one fixture, rules are applied in file/object order. Avoid depending on duplicate or ambiguous ordering across JSON generators; emit specific patterns after broad patterns when order matters.

## 7. Curve rules: `bbb.dmx.curve.v1`

Curve rules transform DMX byte values across channel ranges.

Typical path: `curves/*.json`.

```json
{
  "schema": "bbb.dmx.curve.v1",
  "rules": [
    { "universe": 0, "start": 1, "count": 512, "curve": "gamma", "gamma": 2.2 },
    { "universe": 2, "start": 1, "end": 64, "points": [[0.0, 0.0], [0.2, 0.05], [1.0, 1.0]] }
  ]
}
```

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `schema` | string | yes | — | Must be `bbb.dmx.curve.v1`. |
| `rules` | array | yes | — | Ordered curve rules. Later rules see values modified by earlier matching rules. |

Rule fields:

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `universe` | integer | no | `0` | `0` means all known universes; otherwise exact universe id. |
| `start` | integer | no | `1` | First address. |
| `count` | integer | no | `512` | Number of channels. |
| `end` | integer | no | — | Inclusive end address. If present and positive, overrides `count` as `end - start + 1`. |
| `curve` | string | no | `linear` | `linear`, `gamma`, `invert`, `threshold`, or `points`. |
| `gamma` | number | no | `1.0` | Used by `gamma`; runtime clamps minimum exponent to `0.01`. |
| `threshold` | number | no | `0.5` | Used by `threshold`, clamped to `0..1`. |
| `points` | array of `[x,y]` | no | empty | If present, forces `curve` to `points`. Points are clamped to `0..1` and sorted by `x`. |

## 8. Mask rules: `bbb.dmx.mask.v1`

Mask rules mute, hold, allow, or force channel ranges.

Typical path: `masks/*.json`.

```json
{
  "schema": "bbb.dmx.mask.v1",
  "rules": [
    { "universe": 1, "start": 1, "count": 16, "action": "hold" },
    { "universe": 0, "start": 512, "count": 1, "action": "force", "value": 0 }
  ]
}
```

Rule fields:

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `universe` | integer | no | `0` | `0` means all known universes; otherwise exact universe id. |
| `start` | integer | no | `1` | First address. |
| `count` | integer | no | `512` | Number of channels. |
| `end` | integer | no | — | Inclusive end address. If present and positive, overrides `count`. |
| `action` | string | no | `mute` | `mute`, `hold`, `allow`, or `force`. |
| `value` | integer | no | `0` | Used by `force`, clamped by DMX frame write path to byte range. |

If any rule uses `allow`, the object enters allow-list mode: channels not matched by an `allow` rule are muted after other rule processing. This is deliberately strict; do not mix `allow` casually with broad `mute`/`hold` rules unless you understand the order.

## 9. Assertion rules: `bbb.dmx.assert.v1`

Assertion rules validate DMX frame values for tests and patch guardrails.

Typical path: `asserts/*.json`.

```json
{
  "schema": "bbb.dmx.assert.v1",
  "rules": [
    { "name": "master stays safe", "universe": 1, "start": 1, "end": 4, "min": 0, "max": 255 },
    { "name": "blackout channel", "universe": 1, "channel": 10, "equals": 0 }
  ]
}
```

Rule fields:

| Field | Type | Required | Default | Meaning |
|---|---:|---:|---:|---|
| `name` | string | no | `""` | Human-readable rule name included in failures. |
| `universe` | integer | yes | — | Exact universe id. Unlike curve/mask, `0` is not an all-universe wildcard in canonical assert files. |
| `channel` | integer | either `channel` or `start` | — | Single address shorthand. |
| `start` | integer | either `channel` or `start` | — | First address for a range. |
| `count` | integer | no | `1` | Number of addresses when using `start`. |
| `end` | integer | no | — | Inclusive end address. If present and positive, overrides `count`. |
| `min` | integer | no | — | Minimum allowed byte value. |
| `max` | integer | no | — | Maximum allowed byte value. |
| `equals` | integer | no | — | Exact required byte value. Checked before min/max. |

Canonical assertion rules should include at least one of `min`, `max`, or `equals`. A rule without predicates only increments pass counts and is usually useless.

## 10. Validation workflow for external applications

External producers should validate in three layers:

1. JSON Schema validation against `schemas/*.schema.json`.
2. Runtime validation inside Max with the matching object, because semantic cross-file checks require loaded profiles and patches.
3. Visual/fixture validation with `bbb.dmx.fixtureview`, `bbb.dmx.patchcheck`, and real fixture dry-runs before trusting show output.

Example with `ajv-cli`:

```sh
npx ajv-cli validate -s schemas/bbb.dmx.fixture.profile.v1.schema.json -d fixtures/generic.rgb.3ch.json
npx ajv-cli validate -s schemas/bbb.dmx.patch.v1.schema.json -d patches/rgb-grid.example.json
```

Schema validation catches shape errors. It cannot catch fixture-specific mistakes like a `profile` key that exists in another repository but not in your runtime package, or pan/tilt calibration that is physically wrong.
