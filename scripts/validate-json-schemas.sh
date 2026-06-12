#!/usr/bin/env bash
set -euo pipefail

AJV="${AJV:-npx --yes ajv-cli@5.0.0}"

${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.fixture.profile.v1.schema.json -d 'fixtures/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.patch.v2.schema.json -d 'patches/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.matrixmap.v1.schema.json -d 'maps/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.palette.v1.schema.json -d 'palettes/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.scene.v1.schema.json -d 'scenes/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.curve.v1.schema.json -d 'curves/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.mask.v1.schema.json -d 'masks/*.json'
${AJV} validate --spec=draft2020 --strict=true -s schemas/bbb.dmx.assert.v1.schema.json -d 'asserts/*.json'
