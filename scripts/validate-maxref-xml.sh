#!/usr/bin/env bash
set -euo pipefail
python3 - <<'PY'
import xml.etree.ElementTree as ET
from pathlib import Path
paths = sorted(Path('docs').glob('*.maxref.xml'))
paths += sorted(Path('libs/bbb-dmx/docs').glob('*.maxref.xml'))
for path in paths:
    ET.parse(path)
print(f"validated {len(paths)} maxref XML file(s)")
PY
