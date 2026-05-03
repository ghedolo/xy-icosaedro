### xy-icosaedro

RP2040 firmware that renders a stellated icosahedron on an analog oscilloscope used as an XY vector display. The RP2040 streams stereo audio via a PIO-driven I2S DAC at 96 kHz — left/right channels drive the X and Y deflection inputs. A third digital output (GP5) controls beam blanking during flyback. The icosahedron has 12 base vertices, 20 animated spikes (one per face, oscillating in and out with distributed phases), and 60 edges total. All three rotation axes advance at golden-ratio-derived speeds to produce a non-repeating trajectory.

### Hardware wiring

| RP2040 pin | Signal | Destination |
|---|---|---|
| GP2 | I2S BCLK | DAC BCLK |
| GP3 | I2S LRCLK | DAC LRCLK |
| GP4 | I2S DIN | DAC DIN |
| GP5 | Z blanking | Oscilloscope Z input |

DAC left output → oscilloscope X input
DAC right output → oscilloscope Y input

### Build

```
mkdir -p tmp/build && cd tmp/build
cmake ../.. -DPICO_TOOLCHAIN_PATH=/Users/ghedo/script/AllClaude/rp2040/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin
make -j$(nproc)
```

Flash: `./flash.sh tmp/build/icosaedro.uf2`

### Serial commands (USB CDC, 115200 baud)

Defaults: z_offset=20, scale=20000, spike_min=0.15, spike_max=1.55, rot_speed=1.0×, osc_speed=1.0×, flyback=6

| Key | Action | Range |
|---|---|---|
| `+` / `-` | Z blanking offset | 0–60 |
| `a` / `z` | Scale | 2000–32000 |
| `j` / `n` | Spike max length | > spike_min + 0.10 |
| `k` / `m` | Spike min length | 0.05 – spike_max − 0.10 |
| `s` / `x` | Rotation speed multiplier | 0.0×–5.0× |
| `S` / `X` | Spike oscillation speed multiplier | 0.0×–5.0× |
| `d` / `c` | Flyback steps | 1–40 |
| `r` | Reset all to defaults | — |
| `h` | Print help | — |

### License and Credits

**License:** GPL-3.0-or-later

**Author:** ghedo (luca.ghedini@gmail.com) — 2026

**Development Tool:** The project was constructed using Claude Code by Anthropic.

### Preview

![Stellated icosahedron preview](docs/preview.png)

---

## Development effort

This project was built entirely through a conversation with Claude Code.
The numbers below are extracted from the local session transcripts
(`~/.claude/projects/.../`) and from the git history.

| | |
|---|---|
| First message | 2026-04-29 18:37 UTC |
| Last message | 2026-05-03 13:05 UTC |
| Calendar span | ~3 days |
| Sessions | 4 |
| Commits | 2 |
| Messages | 1679 (680 user + 999 assistant) |
| Active conversation time | ~407 min (~6.8 h) |

> Active time: sum of consecutive message gaps ≤ 5 min (long idle periods excluded).

### Tokens

| Metric | Tokens |
|---|---|
| Input (non-cache) | 2 k |
| Output | 1.2 M |
| Cache write | 1.9 M |
| Cache read | 71.7 M |
| **Total** | **~74.7 M** |
