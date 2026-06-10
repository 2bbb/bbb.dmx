# SPEC.md — `bbb.dmx.movertrack`

## 1. Overview

`bbb.dmx.movertrack` is a Max external that converts a 3D target position into 16-bit DMX pan/tilt values for a moving light.

The object receives:

- the absolute 3D position of the fixture
- optional fixture orientation and mechanical range settings
- a 3D target position

It outputs:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

Each byte is an integer in the range `0..255`.  
Together, the first two bytes represent 16-bit pan, and the last two bytes represent 16-bit tilt.

The object is intended for real-time tracking applications in Max/MSP/Jitter or DMX control patches.

---

## 2. Object Name

```max
bbb.dmx.movertrack
```

---

## 3. Example Usage

```max
[bbb.dmx.movertrack 0. 0. 3. @pan_range 540. @tilt_range 270. @rot 0. 0. 0.]
```

Input target position:

```max
0. 5. 1.5
```

Possible output:

```max
127 255 109 42
```

Meaning:

```text
pan coarse/fine, tilt coarse/fine
```

when `@byte_order coarsefine`.

---

## 4. Coordinate System

The object assumes a right-handed world coordinate system:

```text
+X = stage right
+Y = stage forward
+Z = up
```

The fixture position is given in world coordinates.

The fixture's local default orientation is:

```text
pan 0° direction = local +Y
local +X = right
local +Z = up
```

A target vector is calculated as:

```text
v_world = target_position - fixture_position
```

The fixture rotation attributes define the transform from fixture-local coordinates to world coordinates.  
To aim the fixture, the target vector is converted from world coordinates into fixture-local coordinates.

```text
v_local = inverse(Rz * Ry * Rx) * v_world
```

Because `Rz * Ry * Rx` is a pure rotation matrix, the inverse may be implemented as the transpose.

---

## 5. Constructor Arguments

### Required

```max
[bbb.dmx.movertrack fixture_x fixture_y fixture_z]
```

| Argument | Type | Description |
|---|---:|---|
| `fixture_x` | float | Fixture absolute X position |
| `fixture_y` | float | Fixture absolute Y position |
| `fixture_z` | float | Fixture absolute Z position |

Units are arbitrary but must be consistent across fixture position and target position.  
Meters are the expected practical unit.

---

## 6. Attributes

### `@pan_range`

```max
@pan_range 540.
```

| Type | Default | Unit |
|---|---:|---|
| float | `540.0` | degrees |

Mechanical or DMX-addressable pan range.  
The normalized 16-bit pan range maps to:

```text
-pan_range / 2 ... +pan_range / 2
```

For the default:

```text
-270° ... +270°
```

---

### `@tilt_range`

```max
@tilt_range 270.
```

| Type | Default | Unit |
|---|---:|---|
| float | `270.0` | degrees |

Mechanical or DMX-addressable tilt range.  
The normalized 16-bit tilt range maps to:

```text
-tilt_range / 2 ... +tilt_range / 2
```

For the default:

```text
-135° ... +135°
```

---

### `@rot`

```max
@rot rx ry rz
```

| Type | Default | Unit |
|---|---:|---|
| list of 3 floats | `0. 0. 0.` | degrees |

Fixture orientation in world space.

Rotation order:

```text
R = Rz * Ry * Rx
```

Where:

- `rx` rotates around X
- `ry` rotates around Y
- `rz` rotates around Z

The object converts the world-space target vector to fixture-local space using:

```text
v_local = inverse(R) * v_world
```

---

### `@pan_offset`

```max
@pan_offset 0.
```

| Type | Default | Unit |
|---|---:|---|
| float | `0.0` | degrees |

Offset applied to the calculated pan angle before normalization to DMX.

Use this to calibrate the fixture's DMX pan center against the real-world forward direction.

Processing order:

```text
pan_deg = calculated_pan_deg + pan_offset
```

---

### `@tilt_offset`

```max
@tilt_offset 0.
```

| Type | Default | Unit |
|---|---:|---|
| float | `0.0` | degrees |

Offset applied to the calculated tilt angle before normalization to DMX.

Processing order:

```text
tilt_deg = calculated_tilt_deg + tilt_offset
```

---

### `@pan_invert`

```max
@pan_invert 0
```

| Type | Default |
|---|---:|
| int/bool | `0` |

If non-zero, pan direction is inverted.

Processing order:

```text
if pan_invert:
    pan_deg = -pan_deg
```

This is applied after `@pan_offset`.

---

### `@tilt_invert`

```max
@tilt_invert 0
```

| Type | Default |
|---|---:|
| int/bool | `0` |

If non-zero, tilt direction is inverted.

Processing order:

```text
if tilt_invert:
    tilt_deg = -tilt_deg
```

This is applied after `@tilt_offset`.

---

### `@byte_order`

```max
@byte_order coarsefine
```

| Type | Default | Allowed values |
|---|---|---|
| symbol | `coarsefine` | `coarsefine`, `finecoarse` |

Controls byte order within each 16-bit value.

#### `coarsefine`

```text
pan_coarse pan_fine tilt_coarse tilt_fine
```

Where:

```text
coarse = value16 >> 8
fine   = value16 & 255
```

#### `finecoarse`

```text
pan_fine pan_coarse tilt_fine tilt_coarse
```

Channel order remains pan first, then tilt.  
Only the byte order inside each 16-bit value changes.

---

### `@shortest_pan`

```max
@shortest_pan 1
```

| Type | Default |
|---|---:|
| int/bool | `1` |

Enables continuous pan tracking using the closest equivalent pan angle to the previous output angle.

This must be implemented and must default to enabled.

The purpose is to prevent sudden head flips caused by `atan2()` wrapping between `+180°` and `-180°`.

When enabled, the object keeps internal state:

```text
previous_pan_deg
has_previous_pan
```

On each target update, the object computes the base pan angle and then selects an equivalent angle:

```text
candidate = base_pan_deg + 360 * k
```

where `k` is an integer chosen so that `candidate` is closest to `previous_pan_deg`.

The selected candidate is then clamped to the configured pan range before DMX conversion.

---

## 7. Inlets

### Inlet 1

Accepts target position and control messages.

#### List input

```max
target_x target_y target_z
```

Example:

```max
1.2 4.5 2.0
```

This computes a new pan/tilt value and outputs a 4-integer DMX list.

---

## 8. Outlets

### Outlet 1

Outputs a list of four integers:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

Each integer must be clamped to:

```text
0..255
```

Default byte order:

```text
pan_coarse pan_fine tilt_coarse tilt_fine
```

---

## 9. Supported Messages

### `list`

```max
target_x target_y target_z
```

Computes output for the given target position.

---

### `target`

```max
target target_x target_y target_z
```

Equivalent to list input.  
Useful for explicit routing.

---

### `pos`

```max
pos fixture_x fixture_y fixture_z
```

Updates the fixture absolute position.

Should not reset pan tracking state.

---

### `rot`

```max
rot rx ry rz
```

Updates fixture orientation in degrees.

Should not reset pan tracking state.

---

### `range`

```max
range pan_range tilt_range
```

Updates both mechanical ranges.

Should not reset pan tracking state.

---

### `reset`

```max
reset
```

Clears pan tracking state:

```text
has_previous_pan = false
```

The next target update should initialize tracking from the newly calculated pan angle.

---

### `bang`

```max
bang
```

Recomputes and outputs using the last received target position.

If no target has ever been received, output the neutral/default value:

```text
127 255 127 255
```

for `@byte_order coarsefine`, corresponding approximately to 16-bit value `32767`.

---

## 10. Calculation Pipeline

The complete calculation order must be:

1. Receive target position.
2. Compute world-space vector from fixture to target.
3. Convert vector to fixture-local space using inverse fixture rotation.
4. Compute raw pan/tilt angles from local vector.
5. Apply pan/tilt offsets.
6. Apply pan/tilt inversion.
7. Apply shortest-pan equivalent angle selection.
8. Clamp angles to configured mechanical ranges.
9. Convert angles to unsigned 16-bit values.
10. Split 16-bit values into bytes according to `@byte_order`.
11. Output a 4-integer list.

---

## 11. Pan/Tilt Angle Calculation

Given:

```text
l = v_local
```

Calculate:

```text
pan_rad = atan2(l.x, l.y)
```

Convert to degrees:

```text
pan_deg = rad_to_deg(pan_rad)
```

Calculate horizontal distance:

```text
h = sqrt(l.x * l.x + l.y * l.y)
```

Calculate tilt:

```text
tilt_rad = atan2(l.z, h)
tilt_deg = rad_to_deg(tilt_rad)
```

This produces:

```text
pan_deg  in approximately -180° ... +180°
tilt_deg in approximately -90° ... +90°
```

before offsets, inversion, tracking, and clamping.

---

## 12. Shortest Pan Tracking

### Required behavior

Because `atan2()` wraps at ±180°, the object must avoid unnecessary jumps by choosing the pan angle closest to the previously output pan angle.

Example:

```text
previous_pan_deg = 179°
base_pan_deg     = -179°
```

Naive behavior would jump from `179°` to `-179°`.

With shortest-pan tracking:

```text
candidate angles: -539°, -179°, 181°, 541°, ...
closest to 179°: 181°
```

The object should use:

```text
181°
```

then clamp if needed.

### Algorithm

```c
double choose_shortest_pan(double base_pan_deg, double previous_pan_deg)
{
    double k = round((previous_pan_deg - base_pan_deg) / 360.0);
    return base_pan_deg + 360.0 * k;
}
```

### Initial state

If no previous pan exists:

```text
resolved_pan_deg = base_pan_deg
has_previous_pan = true
```

Then continue normally.

### State update

After clamping, the internal previous pan state should be updated to the clamped pan angle:

```text
previous_pan_deg = clamped_pan_deg
```

Updating with the clamped angle is important because the DMX output cannot represent values beyond the configured mechanical range.

### Disable behavior

If `@shortest_pan 0`, the object should skip equivalent-angle selection and use the base calculated pan angle directly.

Even when disabled, `previous_pan_deg` may still be updated after each output so that re-enabling `@shortest_pan 1` behaves predictably.

---

## 13. Angle Clamping

Pan clamp:

```text
pan_min = -pan_range / 2
pan_max =  pan_range / 2
```

Tilt clamp:

```text
tilt_min = -tilt_range / 2
tilt_max =  tilt_range / 2
```

Clamp:

```c
deg = max(min_deg, min(max_deg, deg));
```

No wrap should be performed after clamping.

---

## 14. 16-bit DMX Conversion

Convert an angle in degrees to unsigned 16-bit DMX value:

```c
uint16_t angle_to_u16(double deg, double range)
{
    double half = range * 0.5;

    if (deg < -half) deg = -half;
    if (deg >  half) deg =  half;

    double normalized = (deg + half) / range;
    int value = (int)round(normalized * 65535.0);

    if (value < 0) value = 0;
    if (value > 65535) value = 65535;

    return (uint16_t)value;
}
```

Mapping:

```text
-range / 2 -> 0
0          -> approximately 32767 or 32768
+range / 2 -> 65535
```

The exact center may be `32767` or `32768` depending on rounding.  
Use `round()` for predictable symmetric behavior.

---

## 15. Byte Splitting

Given:

```c
uint16_t pan16;
uint16_t tilt16;
```

Coarse/fine:

```c
uint8_t pan_coarse  = pan16 >> 8;
uint8_t pan_fine    = pan16 & 255;
uint8_t tilt_coarse = tilt16 >> 8;
uint8_t tilt_fine   = tilt16 & 255;
```

When:

```max
@byte_order coarsefine
```

output:

```text
pan_coarse pan_fine tilt_coarse tilt_fine
```

When:

```max
@byte_order finecoarse
```

output:

```text
pan_fine pan_coarse tilt_fine tilt_coarse
```

---

## 16. Edge Cases

### Target equals fixture position

If the target vector length is nearly zero:

```text
length(v_world) < epsilon
```

where:

```text
epsilon = 1e-9
```

Behavior:

- If a previous valid output exists, output the previous DMX list.
- If no previous valid output exists, output neutral center.

Neutral center is:

```text
pan16  = 32767
tilt16 = 32767
```

or the nearest value produced by the same conversion pipeline for:

```text
pan_deg = 0
tilt_deg = 0
```

---

### Invalid range

If `@pan_range <= 0` or `@tilt_range <= 0`, the object must not divide by zero.

Recommended behavior:

- Clamp invalid range values to `1.0`
- Print a Max warning
- Continue processing

---

### Invalid `@byte_order`

If an unknown symbol is passed:

- keep the previous valid value
- print a Max warning

---

### NaN / Inf input

If any target, position, rotation, range, or offset value is NaN or Inf:

- ignore the message or attribute update
- keep previous valid state
- print a Max warning

---

## 17. Internal State

The object should maintain:

```c
double fixture_x;
double fixture_y;
double fixture_z;

double rot_x_deg;
double rot_y_deg;
double rot_z_deg;

double pan_range_deg;
double tilt_range_deg;

double pan_offset_deg;
double tilt_offset_deg;

int pan_invert;
int tilt_invert;

symbol byte_order;

int shortest_pan;

double last_target_x;
double last_target_y;
double last_target_z;
int has_last_target;

double previous_pan_deg;
int has_previous_pan;

uint16_t last_pan16;
uint16_t last_tilt16;
int has_last_output;
```

---

## 18. Pseudocode

```c
void compute_and_output(double tx, double ty, double tz)
{
    Vec3 target = {tx, ty, tz};
    Vec3 fixture = {fixture_x, fixture_y, fixture_z};

    Vec3 v_world = target - fixture;

    if (length(v_world) < 1e-9) {
        output_previous_or_neutral();
        return;
    }

    Mat3 rx = make_rot_x(deg_to_rad(rot_x_deg));
    Mat3 ry = make_rot_y(deg_to_rad(rot_y_deg));
    Mat3 rz = make_rot_z(deg_to_rad(rot_z_deg));

    Mat3 r = rz * ry * rx;

    // inverse of a rotation matrix
    Vec3 v_local = transpose(r) * v_world;

    double pan_deg = rad_to_deg(atan2(v_local.x, v_local.y));

    double h = sqrt(v_local.x * v_local.x + v_local.y * v_local.y);
    double tilt_deg = rad_to_deg(atan2(v_local.z, h));

    pan_deg += pan_offset_deg;
    tilt_deg += tilt_offset_deg;

    if (pan_invert) {
        pan_deg = -pan_deg;
    }

    if (tilt_invert) {
        tilt_deg = -tilt_deg;
    }

    if (shortest_pan && has_previous_pan) {
        pan_deg = choose_shortest_pan(pan_deg, previous_pan_deg);
    }

    pan_deg = clamp_angle(pan_deg, pan_range_deg);
    tilt_deg = clamp_angle(tilt_deg, tilt_range_deg);

    previous_pan_deg = pan_deg;
    has_previous_pan = 1;

    uint16_t pan16 = angle_to_u16(pan_deg, pan_range_deg);
    uint16_t tilt16 = angle_to_u16(tilt_deg, tilt_range_deg);

    last_pan16 = pan16;
    last_tilt16 = tilt16;
    has_last_output = 1;

    output_dmx_list(pan16, tilt16);
}
```

---

## 19. Expected Test Cases

### Test 1 — Neutral forward

Fixture:

```text
pos = 0 0 0
rot = 0 0 0
target = 0 10 0
```

Expected:

```text
pan_deg = 0
tilt_deg = 0
```

Output should be near center for both pan and tilt.

---

### Test 2 — Right side

Fixture:

```text
pos = 0 0 0
rot = 0 0 0
target = 10 0 0
```

Expected:

```text
pan_deg = 90
tilt_deg = 0
```

---

### Test 3 — Left side

Fixture:

```text
pos = 0 0 0
rot = 0 0 0
target = -10 0 0
```

Expected:

```text
pan_deg = -90
tilt_deg = 0
```

---

### Test 4 — Upward target

Fixture:

```text
pos = 0 0 0
rot = 0 0 0
target = 0 10 10
```

Expected:

```text
pan_deg = 0
tilt_deg = 45
```

---

### Test 5 — Pan shortest tracking across wrap

Sequence:

```text
previous output pan_deg ≈ 179
next base pan_deg ≈ -179
```

With:

```text
@shortest_pan 1
@pan_range 540
```

Expected resolved pan:

```text
181
```

not:

```text
-179
```

---

### Test 6 — Pan clamp

With:

```text
@pan_range 540
```

Valid range:

```text
-270 ... +270
```

Any resolved pan angle above `270` must clamp to `270`.  
Any resolved pan angle below `-270` must clamp to `-270`.

---

### Test 7 — Byte order

For:

```text
pan16  = 0x1234
tilt16 = 0xABCD
```

With:

```text
@byte_order coarsefine
```

Expected:

```text
18 52 171 205
```

With:

```text
@byte_order finecoarse
```

Expected:

```text
52 18 205 171
```

---

## 20. Help Patch Requirements

Create a Max help patch:

```text
bbb.dmx.movertrack.maxhelp
```

It should demonstrate:

1. Basic target input as `x y z`
2. Fixture position constructor arguments
3. `@pan_range` and `@tilt_range`
4. `@rot`
5. `@pan_offset` and `@tilt_offset`
6. `@pan_invert` and `@tilt_invert`
7. `@byte_order coarsefine/finecoarse`
8. `@shortest_pan`
9. Output into four number boxes
10. Example connection to a DMX output object or placeholder list display

---

## 21. Implementation Notes

- Use double precision internally.
- All public numeric inputs may be floats.
- Output values must be Max integers.
- The object should be safe for real-time control-rate use.
- No dynamic memory allocation is required during the normal calculation path.
- The object should avoid printing warnings every frame for repeated invalid input.
- The implementation should be deterministic for identical input/state.
