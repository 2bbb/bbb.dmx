# SPEC.md — `bbb.dmx.movertrack`

## 1. Scope

`bbb.dmx.movertrack` is a Max external that converts a 3D target position into 16-bit DMX pan/tilt bytes for a moving light.

It is one object in the broader `bbb.dmx.*` utility suite. Keep the mover-specific aiming math here; fixture profile and universe mapping belong to `bbb.dmx.fixturemap`.

Output is always four integers:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

Each byte is `0..255`. The first two bytes are 16-bit pan; the last two bytes are 16-bit tilt.

Default byte order is:

```text
pan_coarse pan_fine tilt_coarse tilt_fine
```

## 2. Coordinate system

The world coordinate system is right-handed:

```text
+X = stage right / local right
+Y = stage forward / pan center
+Z = up
```

Fixture position and target position use the same arbitrary unit. Meters are the expected practical unit.

The fixture's local default orientation is:

```text
pan 0° direction = local +Y
local +X = right
local +Z = up
```

The target vector is:

```text
v_world = target_position - fixture_position
```

The fixture rotation attributes define fixture-local to world rotation:

```text
R = Rz * Ry * Rx
v_local = inverse(R) * v_world
```

Because `R` is a pure rotation matrix, the inverse is the transpose.

## 3. Constructor

```max
[bbb.dmx.movertrack fixture_x fixture_y fixture_z]
```

| Argument | Type | Description |
|---|---:|---|
| `fixture_x` | float | Fixture world X position |
| `fixture_y` | float | Fixture world Y position |
| `fixture_z` | float | Fixture world Z position |

## 4. Attributes

| Attribute | Type | Default | Description |
|---|---|---:|---|
| `@fixture_x` | float | `0.` | Fixture world X position |
| `@fixture_y` | float | `0.` | Fixture world Y position |
| `@fixture_z` | float | `0.` | Fixture world Z position |
| `@pan_range` | float | `540.` | DMX-addressable pan range in degrees |
| `@tilt_range` | float | `270.` | DMX-addressable tilt range in degrees |
| `@rot` | list[3] | `0. 0. 0.` | Fixture rotation `rx ry rz` in degrees; order `Rz * Ry * Rx` |
| `@pan_offset` | float | `0.` | Pan offset in degrees |
| `@tilt_offset` | float | `0.` | Tilt offset in degrees |
| `@pan_invert` | bool | `0` | Invert pan after offset |
| `@tilt_invert` | bool | `0` | Invert tilt after offset |
| `@byte_order` | symbol | `coarsefine` | `coarsefine` or `finecoarse` |
| `@tracking_mode` | symbol | `smart` | `smart`, `pan`, or `off` |
| `@shortest_pan` | bool | `1` | Compatibility alias: `1` -> `tracking_mode smart`, `0` -> `tracking_mode off` |

### Range mapping

`@pan_range` and `@tilt_range` map the 16-bit range to:

```text
-range / 2 ... +range / 2
```

For example, `@pan_range 540.` maps to `-270° ... +270°`.

### Calibration and orientation order

The current processing order is:

1. Convert target from world space to fixture-local space using `@rot`.
2. Compute raw pan/tilt from the local vector.
3. Apply `@pan_offset` and `@tilt_offset`.
4. Apply `@pan_invert` and `@tilt_invert`.
5. Resolve tracking mode.
6. Clamp to configured pan/tilt ranges.
7. Convert to 16-bit values.
8. Split bytes according to `@byte_order`.

This order matters. For an upside-down ceiling fixture, prefer correcting physical orientation with `@rot` first, then use offsets for fixture-specific DMX center/horizontal calibration. Do not use `@tilt_invert` as a blind fix for a wrong `@rot`.

Known working upside-down example from the target venue:

```max
[bbb.dmx.movertrack 0. 3.84 2.55 @rot 180. 0. 0. @tilt_invert 0 @tilt_offset -90.]
```

## 5. Messages

### Target input

```max
x y z
target x y z
```

Both forms compute and output a new DMX byte list.

### Fixture position

```max
pos x y z
```

Updates the fixture position. This does not clear tracking history.

### Range

```max
range pan_range tilt_range
```

Updates both ranges. This does not clear tracking history.

### Calibration

```max
calibrate_pan target_x target_y target_z pan_u16
calibrate_tilt target_x target_y target_z tilt_u16
calibrate_pan target_x target_y target_z pan_byte_1 pan_byte_2
calibrate_tilt target_x target_y target_z tilt_byte_1 tilt_byte_2
```

Calibration derives `@pan_offset` or `@tilt_offset` from a known target and a measured desired DMX value. With two byte arguments, the current `@byte_order` is used to combine bytes.

Calibration clears tracking history and then outputs the calibrated target.

### Reset

```max
reset
```

Clears tracking history only. It does not reset attributes, fixture position, offsets, ranges, or byte order.

### Bang

```max
bang
```

Recomputes the last target. If no target has been received, outputs neutral center bytes for the current byte order.

## 6. Pan/tilt math

Given local vector `l = v_local`:

```text
pan_deg = degrees(atan2(l.x, l.y))
h = sqrt(l.x * l.x + l.y * l.y)
tilt_deg = degrees(atan2(l.z, h))
```

Raw angle ranges before calibration/tracking/clamping are approximately:

```text
pan  = -180° ... +180°
tilt =  -90° ...  +90°
```

Angle-to-DMX conversion:

```text
normalized = (clamp(deg, -range/2, range/2) + range/2) / range
value16 = round(normalized * 65535)
```

Neutral center is `32768`.

## 7. Tracking modes

### `tracking_mode smart` — default

`smart` considers multiple physically equivalent candidates before clipping:

- direct pan/tilt
- pan + 180° with tilt flipped across the yoke
- equivalent pan turns by ±360°

It rejects candidates outside the configured pan/tilt ranges, then chooses the smallest pan+tilt movement from the previous output. If no valid candidate exists, it falls back to the calibrated direct solution and then clamps.

This is the correct default for real fixtures because it reduces both unnecessary spins and wrong-direction clipping.

### `tracking_mode pan`

Legacy pan-only shortest-path mode. It chooses the closest equivalent pan angle to the previous pan output:

```text
candidate = base_pan + 360 * round((previous_pan - base_pan) / 360)
```

It does not reason about the tilt flip alternatives before range clipping. It is kept for comparison and backward compatibility, not as the recommended mode.

### `tracking_mode off`

No tracking history. The object uses the direct calibrated pan/tilt solution and then clamps. This is useful for debugging raw math, but it can spin through `atan2()` wrap points.

### `shortest_pan` compatibility

`@shortest_pan` is no longer the primary solver control:

```text
shortest_pan 1 -> tracking_mode smart
shortest_pan 0 -> tracking_mode off
```

Old documentation that describes `shortest_pan 1` as pan-only behavior is obsolete. Use `tracking_mode pan` if you explicitly need that behavior.

## 8. Edge cases

### Target equals fixture position

If the target vector is nearly zero:

- output the previous valid output if one exists
- otherwise output neutral center

### Invalid ranges

Invalid range values (`<= 0`) are sanitized to `1.0` and warned once.

### Invalid symbols

Invalid `@byte_order` or `@tracking_mode` values keep the previous valid value and warn once.

### NaN / Inf

Invalid numeric input is ignored where possible and warned once. Computation falls back to previous output or neutral output as appropriate.

## 9. Outlets

### Outlet 1

Outputs:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

For `@byte_order coarsefine`:

```text
pan_coarse pan_fine tilt_coarse tilt_fine
```

For `@byte_order finecoarse`:

```text
pan_fine pan_coarse tilt_fine tilt_coarse
```

## 10. Expected tests

Minimum behavioral coverage:

1. Forward target: `pos 0 0 0`, `rot 0 0 0`, target `0 10 0` -> pan/tilt near center.
2. Right target: target `10 0 0` -> pan about `+90°`.
3. Left target: target `-10 0 0` -> pan about `-90°`.
4. Up target: target `0 10 10` -> tilt about `+45°`.
5. Byte order: `coarsefine` and `finecoarse` split the same 16-bit values differently.
6. `tracking_mode smart` avoids range-invalid flip candidates before clipping.
7. `tracking_mode pan` preserves legacy pan-only shortest behavior.
8. `tracking_mode off` has no previous-output dependency.
9. `calibrate_pan` and `calibrate_tilt` update offsets and clear tracking history.
10. `reset` clears tracking history without changing configuration.

## 11. Help patch requirements

`help/bbb.dmx.movertrack.maxhelp` must demonstrate:

1. Basic list target input.
2. `target`, `pos`, `range`, `bang`, and `reset` messages.
3. `@rot`, offsets, invert flags, and byte order.
4. `tracking_mode smart|pan|off` and `shortest_pan` compatibility.
5. Offset calibration from known target and DMX value.
6. Upside-down ceiling-hang example using `@rot 180. 0. 0. @tilt_invert 0 @tilt_offset -90.`.
7. Four-byte output and a placeholder connection to a DMX sender.
