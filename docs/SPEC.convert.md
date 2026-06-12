# bbb.dmx converter CLI

`tools/bbb-dmx-convert` is a TypeScript/Node.js CLI that converts external fixture datasets into `bbb.dmx` JSON.

## Supported inputs

- `.gdtf`: ZIP container; reads `description.xml` and converts fixture type DMX modes/channels into `bbb.dmx.fixture.profile.v1`.
- `.mvr`: ZIP container; converts embedded `.gdtf` files and tries to create a `bbb.dmx.patch.v1` from the scene XML.
- GDTF XML: direct `description.xml` input via `--format gdtf-xml`.
- MA3 XML: best-effort fixture XML conversion via `--format ma3`; this only supports exports that expose DMXMode/DMXChannel-like XML nodes.

## Install/build

```sh
cd tools/bbb-dmx-convert
npm install
npm run build
```

Run without building during development only if you provide your own TypeScript runner. Release packages include the TypeScript source and generated `dist/index.js` after `npm run build`.

## Usage

```sh
node tools/bbb-dmx-convert/dist/index.js convert fixture.gdtf \
  --out-dir converted \
  --fixture-dir fixtures \
  --overwrite

node tools/bbb-dmx-convert/dist/index.js convert scene.mvr \
  --out-dir converted \
  --fixture-dir fixtures \
  --patch patches/from-mvr.json \
  --overwrite

node tools/bbb-dmx-convert/dist/index.js convert ma3-fixture.xml \
  --format ma3 \
  --out-dir converted \
  --overwrite
```

## What the converter maps

The converter maps DMX channels into this project’s fixture profile schema:

- DMX offsets become mode `channels[].offset`.
- Known attributes are normalized to parameter keys such as `pan`, `tilt`, `dimmer`, `red`, `green`, `blue`, `white`, `amber`, `uv`, `zoom`, `focus`, `iris`, `gobo`, `color`, etc.
- Repeated attributes in one mode are uniquified (`dimmer`, `dimmer_2`, `dimmer_3`, ...). This prevents duplicate channel keys and keeps each independent DMX channel addressable.
- Multi-byte offsets inside a single GDTF `DMXChannel` become `u16` or `u24` parameters with `coarsefine` / `coarsemidfine` byte order. Separate `DMXChannel` elements with the same attribute are not merged into one multi-byte parameter.
- Pan/tilt physical ranges become `range_degrees` when present in the source XML.
- The first GDTF `<Beam>` geometry node becomes optional profile-level `photometry` metadata (`BeamAngle`, `FieldAngle`, `BeamRadius`, `LuminousFlux`, `ColorTemperature`) when present.
- MVR fixture addresses become `universe` + `address` entries when the scene XML exposes usable address data.
- MVR fixture ids prefer `FixtureID` / `UnitNumber`, then human-readable `Name`, and only fall back to `UUID`; duplicate ids are uniquified with `_2`, `_3`, ... suffixes.
- MVR `<Matrix>` transforms are converted to patch `position` and `rotation`: translation is converted from millimeters to meters, and rotation is decomposed to XYZ Euler degrees matching the `bbb.dmx.movertrack` rotation convention.

## Hard limitation

This is a converter, not a truth oracle. GDTF/MVR/MA3 data in the wild is inconsistent, and MA3 XML exports are especially variable. Every converted file must be checked with:

```max
[bbb.dmx.fixtureinfo]
[bbb.dmx.patchcheck]
[bbb.dmx.fixtureview]
```

Treat warnings as real work, not cosmetic output. If a profile converts but a fixture behaves wrong, the likely failure is attribute normalization or mode/address ambiguity; fix the generated JSON instead of pretending the source format was unambiguous.
