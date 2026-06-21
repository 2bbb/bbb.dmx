#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';
import Ajv2020 from 'ajv/dist/2020.js';

const root = process.cwd();
const schemaDirectory = process.env.BBB_DMX_SCHEMA_DIR || 'libs/bbb-dmx/schemas';

const validations = [
  ['bbb.dmx.fixture.profile.v1.schema.json', 'fixtures'],
  ['bbb.dmx.patch.v2.schema.json', 'patches'],
  ['bbb.dmx.matrixmap.v1.schema.json', 'maps'],
  ['bbb.dmx.palette.v1.schema.json', 'palettes'],
  ['bbb.dmx.scene.v1.schema.json', 'scenes'],
  ['bbb.dmx.curve.v1.schema.json', 'curves'],
  ['bbb.dmx.mask.v1.schema.json', 'masks'],
  ['bbb.dmx.assert.v1.schema.json', 'asserts'],
  ['bbb.dmx.setup.v1.schema.json', 'setups']
];

function readJson(filePath) {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch(error) {
    throw new Error(`failed to read JSON ${filePath}: ${error.message}`);
  }
}

function jsonFiles(directory) {
  if(!fs.existsSync(directory)) {
    throw new Error(`data directory does not exist: ${directory}`);
  }
  return fs.readdirSync(directory)
    .filter((entry) => entry.endsWith('.json'))
    .sort()
    .map((entry) => path.join(directory, entry));
}

function relative(filePath) {
  return path.relative(root, filePath) || filePath;
}

const ajv = new Ajv2020({
  allErrors: true,
  strict: true
});

let validatedCount = 0;
for(const [schemaFileName, dataDirectoryName] of validations) {
  const schemaPath = path.join(root, schemaDirectory, schemaFileName);
  const dataDirectory = path.join(root, dataDirectoryName);
  if(!fs.existsSync(schemaPath)) {
    throw new Error(`schema does not exist: ${schemaPath}`);
  }
  const files = jsonFiles(dataDirectory);
  if(files.length === 0) {
    throw new Error(`no JSON files matched ${dataDirectoryName}/*.json`);
  }

  const validate = ajv.compile(readJson(schemaPath));
  for(const filePath of files) {
    const data = readJson(filePath);
    if(!validate(data)) {
      console.error(`${relative(filePath)} invalid`);
      console.error(ajv.errorsText(validate.errors, {separator: '\n'}));
      process.exitCode = 1;
    } else {
      console.log(`${relative(filePath)} valid`);
      validatedCount++;
    }
  }
}

if(process.exitCode) {
  process.exit(process.exitCode);
}
console.log(`validated ${validatedCount} JSON file(s)`);
