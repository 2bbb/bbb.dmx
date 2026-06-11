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
- Multi-byte offsets become `u16` or `u24` parameters with `coarsefine` / `coarsemidfine` byte order.
- Pan/tilt physical ranges become `range_degrees` when present in the source XML.
- MVR fixture addresses become `universe` + `address` entries when the scene XML exposes usable address data.

## Hard limitation

This is a converter, not a truth oracle. GDTF/MVR/MA3 data in the wild is inconsistent, and MA3 XML exports are especially variable. Every converted file must be checked with:

```max
[bbb.dmx.fixtureinfo]
[bbb.dmx.patchcheck]
[bbb.dmx.fixtureview]
```

Treat warnings as real work, not cosmetic output. If a profile converts but a fixture behaves wrong, the likely failure is attribute normalization or mode/address ambiguity; fix the generated JSON instead of pretending the source format was unambiguous.
