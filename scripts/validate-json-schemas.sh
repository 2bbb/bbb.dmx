#!/usr/bin/env bash
set -euo pipefail

if [ ! -d node_modules/ajv ]; then
  echo "ajv dependency is missing; run: npm ci" >&2
  exit 1
fi

node scripts/validate-json-schemas.mjs
