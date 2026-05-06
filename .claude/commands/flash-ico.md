Build and flash the icosahedro firmware to the RP2040.

Follow these steps exactly, in order:

## 1. Build

```
cd /Users/ghedo/script/AllClaude/rp2040/icosaedro/tmp/build && cmake --build .
```

If the build directory or CMakeCache does not exist, configure first:
```
mkdir -p tmp/build && cd tmp/build
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../toolchain-xpack.cmake
make -j$(nproc)
```

Stop if the build fails — do not proceed to flash.

## 2. Reboot into BOOTSEL

```
picotool reboot -f -u
```

This forces the Pico into BOOTSEL mode via USB. No need to press any button or close CoolTerm.

Requires `dangerouslyDisableSandbox: true` (accesses /Volumes/).

## 3. Wait for the volume

```
sleep 2 && ls /Volumes/RPI-RP2/
```

If the volume is not mounted yet, wait another second and retry. Do not proceed until `RPI-RP2` is visible.

## 4. Copy the UF2

```
cp /Users/ghedo/script/AllClaude/rp2040/icosaedro/tmp/build/icosaedro.uf2 /Volumes/RPI-RP2/
```

Requires `dangerouslyDisableSandbox: true`.

The Pico will reboot automatically after the copy completes.
