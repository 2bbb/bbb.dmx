import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import JSZip from 'jszip';

const root = new URL('..', import.meta.url).pathname;
const temp = join(tmpdir(), 'bbb-dmx-convert-smoke');
rmSync(temp, { recursive: true, force: true });
mkdirSync(temp, { recursive: true });

execFileSync(process.execPath, [join(root, 'dist/index.js'), 'convert', join(root, 'test/minimal.gdtf.xml'), '--format', 'gdtf-xml', '--out-dir', join(temp, 'xml'), '--overwrite'], { stdio: 'inherit' });

const gdtf = new JSZip();
gdtf.file('description.xml', readFileSync(join(root, 'test/minimal.gdtf.xml')));
const gdtfData = await gdtf.generateAsync({ type: 'nodebuffer' });

const mvr = new JSZip();
mvr.file('Tiny RGB Mover.gdtf', gdtfData);
mvr.file('GeneralSceneDescription.xml', `<?xml version="1.0"?>
<GeneralSceneDescription>
  <Scene>
    <Fixtures>
      <Fixture Name="Spot 1" GDTFSpec="Tiny RGB Mover.gdtf" GDTFMode="Basic">
        <Addresses><Address Universe="2" Address="17" /></Addresses>
      </Fixture>
    </Fixtures>
  </Scene>
</GeneralSceneDescription>
`);
writeFileSync(join(temp, 'scene.mvr'), await mvr.generateAsync({ type: 'nodebuffer' }));

execFileSync(process.execPath, [join(root, 'dist/index.js'), 'convert', join(temp, 'scene.mvr'), '--format', 'mvr', '--out-dir', join(temp, 'mvr'), '--patch', 'patches/from-mvr.json', '--overwrite'], { stdio: 'inherit' });
const patch = JSON.parse(readFileSync(join(temp, 'mvr/patches/from-mvr.json'), 'utf8'));
if(patch.fixtures?.[0]?.universe !== 2 || patch.fixtures?.[0]?.address !== 17) {
  throw new Error('MVR smoke patch did not preserve universe/address');
}
