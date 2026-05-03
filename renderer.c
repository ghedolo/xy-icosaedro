#include "renderer.h"
#include <string.h>
#include <stdbool.h>

#define FLYBACK_STEPS  6
#define DRAW_STEPS     2    // budget: 32 + 41*(6+20) + 60*2 = 32+1066+120 = 1218 < 1536

#define Z_ON   0x80000000u
#define Z_OFF  0x00000000u

volatile int z_offset = 20;

static inline uint32_t pack_xy(int32_t x, int32_t y) {
    if (x >  32767) x =  32767; else if (x < -32767) x = -32767;
    if (y >  32767) y =  32767; else if (y < -32767) y = -32767;
    return ((uint32_t)(uint16_t)(int16_t)y << 16) | (uint16_t)(int16_t)x;
}

static int write_segment(frame_t *f, int pos,
                          int32_t x0, int32_t y0,
                          int32_t x1, int32_t y1,
                          int steps, bool beam_on) {
    uint32_t zval = beam_on ? Z_ON : Z_OFF;
    for (int i = 0; i < steps && pos < SAMPLES_PER_FRAME; i++, pos++) {
        int32_t x = x0 + (x1 - x0) * i / steps;
        int32_t y = y0 + (y1 - y0) * i / steps;
        f->audio[pos] = pack_xy(x, y);
        f->z[pos]     = zval;
    }
    return pos;
}

void renderer_render(frame_t *f,
                     const int16_t verts_2d[][2],
                     const int edges[][2], int edge_count) {
    int guard = z_offset;
    if (guard < 0)  guard = 0;
    if (guard > 60) guard = 60;

    int pos = 0;
    int32_t cx = 0, cy = 0;

    for (int i = 0; i < 32; i++, pos++) {
        f->audio[pos] = pack_xy(0, 0);
        f->z[pos]     = Z_OFF;
    }

    for (int e = 0; e < edge_count && pos < SAMPLES_PER_FRAME; e++) {
        int32_t x0 = verts_2d[edges[e][0]][0];
        int32_t y0 = verts_2d[edges[e][0]][1];
        int32_t x1 = verts_2d[edges[e][1]][0];
        int32_t y1 = verts_2d[edges[e][1]][1];

        if (x0 != cx || y0 != cy) {
            pos = write_segment(f, pos, cx, cy, x0, y0, FLYBACK_STEPS, false);
            pos = write_segment(f, pos, x0, y0, x0, y0, guard, false);
        }

        pos = write_segment(f, pos, x0, y0, x1, y1, DRAW_STEPS, true);
        cx = x1;
        cy = y1;
    }

    while (pos < SAMPLES_PER_FRAME) {
        f->audio[pos] = pack_xy(cx, cy);
        f->z[pos]     = Z_OFF;
        pos++;
    }

    if (guard > 0) {
        memmove(f->z + guard, f->z, (SAMPLES_PER_FRAME - guard) * sizeof(uint32_t));
        for (int i = 0; i < guard; i++)
            f->z[i] = Z_OFF;
    }
}
