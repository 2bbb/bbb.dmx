import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import JSZip from 'jszip';

const root = new URL('..', import.meta.url).pathname;
const temp = join(tmpdir(), 'bbb-dmx-convert-smoke');
rmSync(temp, { recursive: true, force: true });
mkdirSync(temp, { recursive: true });

function run(args) {
  execFileSync(process.execPath, [join(root, 'dist/index.js'), ...args], { stdio: 'inherit' });
}

function readJson(file) {
  return JSON.parse(readFileSync(file, 'utf8'));
}

function assertClose(actual, expected, label, epsilon = 1.0e-6) {
  if(Math.abs(actual - expected) > epsilon) {
    throw new Error(`${label}: expected ${expected}, got ${actual}`);
  }
}

run(['convert', join(root, 'test/minimal.gdtf.xml'), '--format', 'gdtf-xml', '--out-dir', join(temp, 'xml'), '--overwrite']);

const repeatedAttributeXml = `<?xml version="1.0" encoding="UTF-8"?>
<GDTF>
  <FixtureType Name="Two Dimmer" LongName="Two Dimmer" Manufacturer="ExampleCo">
    <DMXModes>
      <DMXMode Name="Basic">
        <DMXChannels>
          <DMXChannel Offset="1"><LogicalChannel Attribute="Dimmer"><ChannelFunction Attribute="Dimmer" Default="0/1" /></LogicalChannel></DMXChannel>
          <DMXChannel Offset="2"><LogicalChannel Attribute="Dimmer"><ChannelFunction Attribute="Dimmer" Default="0/1" /></LogicalChannel></DMXChannel>
        </DMXChannels>
      </DMXMode>
    </DMXModes>
  </FixtureType>
</GDTF>
`;
writeFileSync(join(temp, 'repeated.gdtf.xml'), repeatedAttributeXml);
run(['convert', join(temp, 'repeated.gdtf.xml'), '--format', 'gdtf-xml', '--out-dir', join(temp, 'repeated'), '--overwrite']);
const repeatedProfile = readJson(join(temp, 'repeated/fixtures/exampleco.two.dimmer.json'));
const repeatedMode = repeatedProfile.modes.basic;
const channelKeys = repeatedMode.channels.map((channel) => channel.key);
if(new Set(channelKeys).size !== channelKeys.length) {
  throw new Error(`repeated attribute conversion emitted duplicate channel keys: ${channelKeys.join(', ')}`);
}
if(channelKeys[0] !== 'dimmer' || channelKeys[1] !== 'dimmer_2') {
  throw new Error(`repeated attribute channel keys were not uniquified as expected: ${channelKeys.join(', ')}`);
}
if(repeatedMode.parameters.dimmer?.channel !== 'dimmer' || repeatedMode.parameters.dimmer_2?.channel !== 'dimmer_2') {
  throw new Error('repeated attribute parameters do not address their own unique channel keys');
}

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
        <Matrix>{1.000000,0.000000,0.000000}{0.000000,-1.000000,0.000000}{0.000000,0.000000,-1.000000}{-2000.000000,1500.000000,4000.000000}</Matrix>
        <Addresses><Address Universe="2" Address="17" /></Addresses>
      </Fixture>
    </Fixtures>
  </Scene>
</GeneralSceneDescription>
`);
writeFileSync(join(temp, 'scene.mvr'), await mvr.generateAsync({ type: 'nodebuffer' }));

run(['convert', join(temp, 'scene.mvr'), '--format', 'mvr', '--out-dir', join(temp, 'mvr'), '--patch', 'patches/from-mvr.json', '--overwrite']);
const patch = readJson(join(temp, 'mvr/patches/from-mvr.json'));
const fixture = patch.fixtures?.[0];
if(fixture?.universe !== 2 || fixture?.address !== 17) {
  throw new Error('MVR smoke patch did not preserve universe/address');
}
assertClose(fixture.position[0], -2.0, 'MVR matrix position.x');
assertClose(fixture.position[1], 1.5, 'MVR matrix position.y');
assertClose(fixture.position[2], 4.0, 'MVR matrix position.z');
assertClose(fixture.rotation[0], 180.0, 'MVR matrix rotation.x');
assertClose(fixture.rotation[1], 0.0, 'MVR matrix rotation.y');
assertClose(fixture.rotation[2], 0.0, 'MVR matrix rotation.z');
