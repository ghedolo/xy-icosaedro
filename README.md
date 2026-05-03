# xy-icosaedro

RP2040 firmware that renders a stellated icosahedron on an analog oscilloscope used as an XY vector display. The RP2040 streams stereo audio via a PIO-driven I2S DAC at 96 kHz — left/right channels drive the X and Y deflection inputs. A third digital output (GP5) controls beam blanking during flyback. The icosahedron has 12 base vertices, 20 animated spikes (one per face, oscillating in and out with distributed phases), and 60 edges total. All three rotation axes advance at golden-ratio-derived speeds to produce a non-repeating trajectory.

## Hardware wiring

| RP2040 pin | Signal | Destination |
|---|---|---|
| GP2 | I2S BCLK | DAC BCLK |
| GP3 | I2S LRCLK | DAC LRCLK |
| GP4 | I2S DIN | DAC DIN |
| GP5 | Z blanking | Oscilloscope Z input |

DAC left output → oscilloscope X input  
DAC right output → oscilloscope Y input

## Build

```
mkdir -p tmp/build && cd tmp/build
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../toolchain-xpack.cmake
make -j$(nproc)
```

Flash: `./flash.sh tmp/build/icosaedro.uf2`

## Serial commands (USB CDC, 115200 baud)

| Key | Effect |
|---|---|
| `+` / `=` | Increase `z_offset` (beam blanking guard, 0–60) |
| `-` | Decrease `z_offset` |
| `a` | Increase scale (+500, max 32000) |
| `z` | Decrease scale (−500, min 2000) |
| `j` | Increase `spike_max` (+0.05, max 2.50) |
| `n` | Decrease `spike_max` (−0.05) |
| `k` | Increase `spike_min` (+0.05) |
| `m` | Decrease `spike_min` (−0.05, min 0.05) |

Defaults: `z_offset = 20`, `scale = 20000`, `spike_min = 0.15`, `spike_max = 1.55`.

## Preview

![Stellated icosahedron preview](docs/preview.png)

---

## Development effort

This project was built entirely through a conversation with Claude Code.
The numbers below are extracted from the local session transcripts
(`~/.claude/projects/.../`) and from the git history.

| | |
|---|---|
| First message | 2026-04-29 18:37 UTC |
| Last message | 2026-05-03 09:27 UTC |
| Calendar span | ~3 days |
| Sessions | 4 |
| Commits | 0 |
| Messages | 1376 (555 user + 821 assistant) |
| Active conversation time | ~343 min (~5.7 h) |

> Active time: sum of consecutive message gaps ≤ 5 min (long idle periods excluded).

### Tokens

| Metric | Tokens |
|---|---|
| Input (non-cache) | 1 k |
| Output | 1.1 M |
| Cache write | 1.6 M |
| Cache read | 56.5 M |
| **Total** | **~59.2 M** |
