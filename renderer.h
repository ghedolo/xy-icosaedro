#pragma once
#include <stdint.h>

// 1536 campioni per frame → ~62 Hz refresh
#define SAMPLES_PER_FRAME  1536

typedef struct {
    uint32_t audio[SAMPLES_PER_FRAME];
    uint32_t z[SAMPLES_PER_FRAME];   // un word per campione; bit 31 = fascio acceso
} frame_t;

extern volatile int z_offset;
extern volatile int flyback_steps;

void renderer_render(frame_t *f,
                     const int16_t verts_2d[][2],
                     const int edges[][2], int edge_count);
