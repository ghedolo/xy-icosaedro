#include <math.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "audio_i2s.pio.h"
#include "renderer.h"

#define I2S_DIN_PIN   4
#define I2S_BCLK_PIN  2
#define Z_PIN         5

#define PIO_INST  pio0
#define I2S_SM    0
#define Z_SM      1
#define AUDIO_DMA 0
#define Z_DMA     1

#define SAMPLE_RATE 96000

// ---------------------------------------------------------------------------
// Geometria: icosaedro stellato
// ---------------------------------------------------------------------------

#define PHI     1.6180339887f   // rapporto aureo
#define N_BASE  12
#define N_FACES 20
#define N_VERTS (N_BASE + N_FACES)   // 32: 12 base + 20 punte
#define N_EDGES (N_FACES * 3)         // 60 spigoli (3 per punta)
#define DEF_SCALE       32000.0f
#define DEF_SPIKE_MAX   1.0f
#define DEF_SPIKE_MIN   1.0f
#define BASE_SPIKE_RATE 0.018f   // rate base (rad/frame) a moltiplicatore 1.0×
#define DEF_SPIKE_SPEED 1.0f     // moltiplicatore default
#define DEF_ROT_SPEED   1.0f
#define DEF_FLYBACK     10
#define DEF_Z_OFFSET    20

static float g_scale       = DEF_SCALE;
static float g_spike_max   = DEF_SPIKE_MAX;
static float g_spike_min   = DEF_SPIKE_MIN;
static float g_spike_speed = DEF_SPIKE_SPEED;
static float g_rot_speed   = DEF_ROT_SPEED;

// Vertici dell'icosaedro regolare (non normalizzati)
static const float ico_base[N_BASE][3] = {
    { 0,     1,     PHI}, { 0,    -1,     PHI},
    { 0,     1,    -PHI}, { 0,    -1,    -PHI},
    { 1,     PHI,   0  }, {-1,     PHI,   0  },
    { 1,    -PHI,   0  }, {-1,    -PHI,   0  },
    { PHI,   0,     1  }, {-PHI,   0,     1  },
    { PHI,   0,    -1  }, {-PHI,   0,    -1  },
};

// 20 facce triangolari dell'icosaedro
static const int ico_faces[N_FACES][3] = {
    {0,4,8}, {0,8,1}, {0,1,9}, {0,9,5}, {0,5,4},     // calotta superiore
    {3,10,2},{3,2,11},{3,11,7},{3,7,6}, {3,6,10},     // calotta inferiore
    {4,2,10},{4,10,8},{8,10,6},{8,6,1}, {1,6,7},      // banda alta
    {1,7,9}, {9,7,11},{9,11,5},{5,11,2},{5,2,4},      // banda bassa
};

static float   verts_3d[N_VERTS][3];
static float   face_dir[N_FACES][3];   // direzione normalizzata di ogni punta
static int     edges[N_EDGES][2];
static int16_t projected[N_VERTS][2];
static float   spike_phase[N_FACES];
static float   spike_t = 0.0f;

static void build_stellated(void) {
    for (int i = 0; i < N_BASE; i++) {
        float x = ico_base[i][0], y = ico_base[i][1], z = ico_base[i][2];
        float len = sqrtf(x*x + y*y + z*z);
        verts_3d[i][0] = x/len;
        verts_3d[i][1] = y/len;
        verts_3d[i][2] = z/len;
    }
    for (int f = 0; f < N_FACES; f++) {
        int a = ico_faces[f][0], b = ico_faces[f][1], c = ico_faces[f][2];
        float cx = verts_3d[a][0]+verts_3d[b][0]+verts_3d[c][0];
        float cy = verts_3d[a][1]+verts_3d[b][1]+verts_3d[c][1];
        float cz = verts_3d[a][2]+verts_3d[b][2]+verts_3d[c][2];
        float len = sqrtf(cx*cx + cy*cy + cz*cz);
        face_dir[f][0] = cx/len;
        face_dir[f][1] = cy/len;
        face_dir[f][2] = cz/len;
        spike_phase[f] = (f % 3) * (6.2831853f / 3.0f);  // 0°, 120°, 240° dentro ogni terzetto
        edges[f*3+0][0] = a;       edges[f*3+0][1] = N_BASE+f;
        edges[f*3+1][0] = N_BASE+f; edges[f*3+1][1] = b;
        edges[f*3+2][0] = c;       edges[f*3+2][1] = N_BASE+f;
    }
}

static void update_spikes(void) {
    spike_t += BASE_SPIKE_RATE * g_spike_speed;
    for (int f = 0; f < N_FACES; f++) {
        float s = g_spike_min + (g_spike_max - g_spike_min) *
                  (1.0f + sinf(spike_t + spike_phase[f])) * 0.5f;
        verts_3d[N_BASE+f][0] = face_dir[f][0] * s;
        verts_3d[N_BASE+f][1] = face_dir[f][1] * s;
        verts_3d[N_BASE+f][2] = face_dir[f][2] * s;
    }
}

// ---------------------------------------------------------------------------
// Proiezione
// ---------------------------------------------------------------------------

static float rot_y = 0.0f;
static float rot_x = 0.0f;
static float rot_z = 0.0f;

// Velocità base di rotazione (rad/frame); scalate da g_rot_speed
#define BASE_DELTA_Y  0.00360f
#define BASE_DELTA_X  0.00222f   // BASE_DELTA_Y / PHI
#define BASE_DELTA_Z  0.00137f   // BASE_DELTA_Y / PHI^2

static void project_vertices(void) {
    float cy = cosf(rot_y), sy = sinf(rot_y);
    float cx = cosf(rot_x), sx = sinf(rot_x);
    float cz = cosf(rot_z), sz = sinf(rot_z);
    // Matrice Ry * Rx * Rz
    float m00 =  cy*cz + sy*sx*sz;
    float m01 = -cy*sz + sy*sx*cz;
    float m02 =  sy*cx;
    float m10 =  cx*sz;
    float m11 =  cx*cz;
    float m12 = -sx;
    for (int i = 0; i < N_VERTS; i++) {
        float x = verts_3d[i][0];
        float y = verts_3d[i][1];
        float z = verts_3d[i][2];
        projected[i][0] = (int16_t)((m00*x + m01*y + m02*z) * g_scale);
        projected[i][1] = (int16_t)((m10*x + m11*y + m12*z) * g_scale);
    }
}

// ---------------------------------------------------------------------------
// Double buffer
// ---------------------------------------------------------------------------

static frame_t frames[2];
static volatile int  playing_buf  = 0;
static volatile bool swap_pending = false;

static void dma_irq_handler(void) {
    if (!dma_channel_get_irq0_status(AUDIO_DMA)) return;
    dma_channel_acknowledge_irq0(AUDIO_DMA);
    playing_buf ^= 1;
    dma_channel_abort(Z_DMA);
    dma_channel_set_read_addr(Z_DMA,     frames[playing_buf].z,     true);
    dma_channel_set_read_addr(AUDIO_DMA, frames[playing_buf].audio, true);
    swap_pending = true;
}

// ---------------------------------------------------------------------------
// Init HW
// ---------------------------------------------------------------------------

static void i2s_init(void) {
    PIO pio = PIO_INST; uint sm = I2S_SM;
    uint off = pio_add_program(pio, &audio_i2s_program);
    pio_sm_config c = audio_i2s_program_get_default_config(off);
    sm_config_set_out_pins(&c, I2S_DIN_PIN, 1);
    sm_config_set_sideset_pins(&c, I2S_BCLK_PIN);
    sm_config_set_out_shift(&c, false, true, 32);
    float clkdiv = (float)clock_get_hz(clk_sys) / (64.0f * SAMPLE_RATE);
    sm_config_set_clkdiv(&c, clkdiv);
    pio_gpio_init(pio, I2S_DIN_PIN);
    pio_gpio_init(pio, I2S_BCLK_PIN);
    pio_gpio_init(pio, I2S_BCLK_PIN + 1);
    for (uint p = I2S_BCLK_PIN; p <= I2S_BCLK_PIN + 1; p++) {
        gpio_set_slew_rate(p, GPIO_SLEW_RATE_SLOW);
        gpio_set_drive_strength(p, GPIO_DRIVE_STRENGTH_2MA);
    }
    gpio_set_slew_rate(I2S_DIN_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_drive_strength(I2S_DIN_PIN, GPIO_DRIVE_STRENGTH_2MA);
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_DIN_PIN,  1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_BCLK_PIN, 2, true);
    pio_sm_init(pio, sm, off, &c);
    pio_sm_exec(pio, sm, pio_encode_set(pio_x, 14));
    pio_sm_set_enabled(pio, sm, true);
}

static void zaxis_init(void) {
    PIO pio = PIO_INST; uint sm = Z_SM;
    uint off = pio_add_program(pio, &zaxis_program);
    pio_sm_config c = zaxis_program_get_default_config(off);
    sm_config_set_out_pins(&c, Z_PIN, 1);
    sm_config_set_out_shift(&c, false, true, 32);
    float i2s_raw = (float)clock_get_hz(clk_sys) / (64.0f * SAMPLE_RATE);
    uint32_t i2s_int  = (uint32_t)i2s_raw;
    uint32_t i2s_frac = (uint32_t)((i2s_raw - i2s_int) * 256.0f + 0.5f);
    uint32_t z_total  = (i2s_int * 256 + i2s_frac) * 32;
    sm_config_set_clkdiv_int_frac(&c, z_total / 256, z_total % 256);
    gpio_init(Z_PIN); gpio_set_dir(Z_PIN, GPIO_OUT); gpio_put(Z_PIN, 0);
    pio_gpio_init(pio, Z_PIN);
    pio_sm_set_consecutive_pindirs(pio, sm, Z_PIN, 1, true);
    pio_sm_init(pio, sm, off, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static void dma_init(void) {
    PIO pio = PIO_INST;
    {
        dma_channel_config c = dma_channel_get_default_config(AUDIO_DMA);
        channel_config_set_dreq(&c, pio_get_dreq(pio, I2S_SM, true));
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        dma_channel_configure(AUDIO_DMA, &c,
            (void *)&pio->txf[I2S_SM], frames[0].audio, SAMPLES_PER_FRAME, false);
        dma_channel_set_irq0_enabled(AUDIO_DMA, true);
    }
    {
        dma_channel_config c = dma_channel_get_default_config(Z_DMA);
        channel_config_set_dreq(&c, pio_get_dreq(pio, Z_SM, true));
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        dma_channel_configure(Z_DMA, &c,
            (void *)&pio->txf[Z_SM], frames[0].z, SAMPLES_PER_FRAME, false);
    }
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

static void print_help(void) {
    printf("icosaedro stellato — ghedo 05/2026\n");
    printf("  +/-   z_offset (%d, 0-60)\n",        z_offset);
    printf("  a/z   scale    (%.0f, 2000-32767)\n", g_scale);
    printf("  j/n   spike_max (%.2f, >=spike_min, max 2.50)\n", g_spike_max);
    printf("  k/m   spike_min (%.2f, 0.05-spike_max)\n", g_spike_min);
    printf("  s/x   rot speed  (%.2fx, 0.0-5.0)\n",  g_rot_speed);
    printf("  S/X   osc speed  (%.2fx, 0.0-5.0)\n",  g_spike_speed);
    printf("  d/c   flyback steps (%d, 1-40)\n",    flyback_steps);
    printf("  r     reset defaults\n");
    printf("  h     this help\n");
}

int main(void) {
    stdio_init_all();
    build_stellated();
    project_vertices();

    renderer_render(&frames[0], (const int16_t (*)[2])projected,
                    (const int (*)[2])edges, N_EDGES);
    renderer_render(&frames[1], (const int16_t (*)[2])projected,
                    (const int (*)[2])edges, N_EDGES);

    i2s_init();
    zaxis_init();
    dma_init();

    dma_channel_start(Z_DMA);
    dma_channel_start(AUDIO_DMA);

    print_help();

    while (true) {
        int ch = getchar_timeout_us(0);
        bool changed = true;
        if (ch == 'h') {
            print_help();
            changed = false;
        } else if (ch == 'r') {
            z_offset       = DEF_Z_OFFSET;
            g_scale        = DEF_SCALE;
            g_spike_max    = DEF_SPIKE_MAX;
            g_spike_min    = DEF_SPIKE_MIN;
            g_spike_speed  = DEF_SPIKE_SPEED;
            g_rot_speed    = DEF_ROT_SPEED;
            flyback_steps  = DEF_FLYBACK;
            print_help();
        } else if (ch == '+' || ch == '=') {
            if (z_offset < 60) z_offset++;
        } else if (ch == '-') {
            if (z_offset > 0)  z_offset--;
        } else if (ch == 'a') {
            if (g_scale < 32767.0f) g_scale += 500.0f;
        } else if (ch == 'z') {
            if (g_scale > 2000.0f)  g_scale -= 500.0f;
        } else if (ch == 'j') {
            if (g_spike_max < 2.50f) g_spike_max += 0.05f;
        } else if (ch == 'n') {
            if (g_spike_max > g_spike_min) g_spike_max -= 0.05f;
        } else if (ch == 'k') {
            if (g_spike_min < g_spike_max) g_spike_min += 0.05f;
        } else if (ch == 'm') {
            if (g_spike_min > 0.05f) g_spike_min -= 0.05f;
        } else if (ch == 's') {
            if (g_rot_speed < 5.0f) g_rot_speed += 0.1f;
        } else if (ch == 'x') {
            if (g_rot_speed >= 0.1f) g_rot_speed -= 0.1f;
        } else if (ch == 'S') {
            if (g_spike_speed < 5.0f) g_spike_speed += 0.1f;
        } else if (ch == 'X') {
            if (g_spike_speed >= 0.1f) g_spike_speed -= 0.1f;
        } else if (ch == 'd') {
            if (flyback_steps < 40) flyback_steps++;
        } else if (ch == 'c') {
            if (flyback_steps > 1)  flyback_steps--;
        } else {
            changed = false;
        }
        if (changed && ch != 'r') {
            printf("z=%d  sc=%.0f  spk=%.2f/%.2f  rot=%.2fx  osc=%.2fx  fb=%d\n",
                   z_offset, g_scale, g_spike_min, g_spike_max,
                   g_rot_speed, g_spike_speed, flyback_steps);
        }

        if (swap_pending) {
            swap_pending = false;
            rot_y += g_rot_speed * BASE_DELTA_Y;
            rot_x += g_rot_speed * BASE_DELTA_X;
            rot_z += g_rot_speed * BASE_DELTA_Z;
            update_spikes();
            project_vertices();
            int back = 1 - playing_buf;
            renderer_render(&frames[back],
                            (const int16_t (*)[2])projected,
                            (const int (*)[2])edges, N_EDGES);
        }
    }
}
