# bbb.dmx

DMX utility external object suite for Max/MSP.

Current object:

- `bbb.dmx.movertrack` — converts a 3D target position into 16-bit DMX pan/tilt bytes for a moving light.

## Build

```sh
git submodule update --init --recursive
cmake -B build -DBBB_DMX_BUILD_EXTERNALS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

macOS builds a universal `.mxo` (`x86_64` + `arm64`). Windows builds `.mxe64` via Visual Studio 2022.

## `bbb.dmx.movertrack`

```max
[bbb.dmx.movertrack 0. 0. 3. @pan_range 540. @tilt_range 270. @rot 0. 0. 0.]
```

Input a target position list:

```max
0. 5. 1.5
```

Output:

```text
pan_byte_1 pan_byte_2 tilt_byte_1 tilt_byte_2
```

Default byte order is `coarsefine`.
