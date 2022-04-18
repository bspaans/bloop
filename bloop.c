#define SOKOL_IMPL
#include <sokol_audio.h>
#include <stdio.h>
#include <math.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"

sg_pass_action pass_action;

typedef struct bloop_generator{
    float (*fn)(void*, int);
    void *userData;
} bloop_generator;

bloop_generator* bloop_new_generator(float (*fn)(void*, int), void *userData) {
    bloop_generator *closure = malloc(sizeof(*closure));
    closure->fn = fn;
    closure->userData = userData;
    return closure;
}

#define bloop_run(closure, tick) ((*closure->fn)(closure->userData, tick))

int SAMPLE_RATE = 44100;

typedef struct bloop_sine_wave_data {
    bloop_generator *pitch;
    bloop_generator *gain;
    float phase;
} bloop_sine_wave_data;

float bloop_sine_wave_(void *value, int tick) {
    bloop_sine_wave_data *data = (bloop_sine_wave_data *) value;
    float pitch = bloop_run(data->pitch, tick);
    float p = fmin(fmax(pitch, 0.0), SAMPLE_RATE/2.0);
    float step_size = (p * 2 * M_PI) / (float) SAMPLE_RATE;
    float result = sin(data->phase) * bloop_run(data->gain, tick);
    data->phase += step_size;
    return result;
}

bloop_generator *bloop_sine_wave(bloop_generator *pitch, bloop_generator *gain) {
    bloop_sine_wave_data *v = malloc(sizeof(bloop_sine_wave_data));
    v->pitch = pitch;
    v->gain = gain;
    return bloop_new_generator(bloop_sine_wave_, v);
}

float bloop_constant_(void *value, int tick) {
    float *v = (float *)value;
    return *v;
}

bloop_generator *bloop_constant(float value) {
    float *v = malloc(sizeof(float));
    *v = value;
    return bloop_new_generator(bloop_constant_, v); 
}

#define C(c) (bloop_constant(c))

typedef struct bloop_interpolation_data {
    float from;
    float to;
    int over;
} bloop_interpolation_data;

float bloop_interpolation_(void *value, int tick) {
    bloop_interpolation_data *data = (bloop_interpolation_data*) value;
    if (tick >= data->over) {
        return data->to;
    }
    float stepSize = (data->to - data->from) / ((float)data->over);
    return ((float)tick) * stepSize + data->from;
}

bloop_generator *bloop_interpolation(float from, float to, int over) {
    bloop_interpolation_data *v = malloc(sizeof(*v));
    v->from = from;
    v->to = to;
    v->over = over;
    return bloop_new_generator(bloop_interpolation_, v);
}

typedef struct bloop_adsr_data {
    float max_gain;
    float sustain;

    int attack_samples;
    int decay_samples;
    int sustain_samples;
    int release_samples;
} bloop_adsr_data;

float bloop_adsr_(void *value, int tick) {
    bloop_adsr_data *data = (bloop_adsr_data*)value;
    if (tick <= data->attack_samples) {
        float step_size = data->max_gain / ((float)data->attack_samples);
        return ((float)tick) * step_size;
    }

    if (tick <= data->attack_samples + data->decay_samples) {
        float step_size = (data->max_gain - data->sustain) / ((float)data->decay_samples);
        return data->max_gain - (((float)tick-data->attack_samples) * step_size);
    }

    if (tick <= data->attack_samples + data->decay_samples + data->sustain_samples) {
        return data->sustain;
    }


    if (tick <= data->attack_samples + data->decay_samples + data->sustain_samples + data->release_samples) {
        float step_size = data->sustain / ((float)data->release_samples);
        int offset = data->attack_samples + data->decay_samples + data->sustain_samples;
        return data->sustain - (((float)(tick - offset)) * step_size);
    }

    return 0.0;
}

bloop_generator *bloop_adsr(float max_gain, float sustain, int attack_samples, int decay_samples, int sustain_samples, int release_samples) {
    bloop_adsr_data *v = malloc(sizeof(*v));
    v->max_gain = max_gain;
    v->sustain = sustain;
    v->attack_samples = attack_samples;
    v->decay_samples = decay_samples;
    v->sustain_samples = sustain_samples;
    v->release_samples = release_samples;
    return bloop_new_generator(bloop_adsr_, v);
}


typedef struct bloop_lfo_data {
    bloop_generator *speed;
    bloop_generator *offset;
    bloop_generator *amount;
} bloop_lfo_data;

float bloop_lfo_(void *value, int tick) {
    bloop_lfo_data *data = (bloop_lfo_data*)value;
    float speed = bloop_run(data->speed, tick);
    float offset = bloop_run(data->offset, tick);
    float amount = bloop_run(data->amount, tick);

    float stepSize = (speed * M_PI * 2) / (float) SAMPLE_RATE;
    return sin((float)tick * stepSize) * amount + offset;
}

bloop_generator *bloop_lfo(bloop_generator *speed, bloop_generator *offset, bloop_generator *amount) {
    bloop_lfo_data *v = malloc(sizeof(bloop_lfo_data));
    v->speed = speed;
    v->offset = offset;
    v->amount = amount;
    return bloop_new_generator(bloop_lfo_, v);
}

#define LFO(speed, offset, amount) (bloop_lfo(bloop_constant(speed), bloop_constant(offset), bloop_constant(amount)))

typedef struct bloop_distortion_data {
    bloop_generator *input;
    bloop_generator *level;
    bloop_generator *gain;
} bloop_distortion_data;

float bloop_distortion_(void *value, int tick) {
    bloop_distortion_data *data = (bloop_distortion_data*) value;
    float s = bloop_run(data->input, tick);
    float lvl = bloop_run(data->level, tick);
    float gain = bloop_run(data->gain, tick);
    if (s >= lvl) {
        s = lvl;
    } else if (s <= -1*lvl) {
        s = -1 * lvl;
    }
    return s * gain;
}

bloop_generator *bloop_distortion(bloop_generator *input, bloop_generator *level, bloop_generator *gain) {
    bloop_distortion_data *v = malloc(sizeof(bloop_distortion_data));
    v->input = input;
    v->level = level;
    v->gain = gain;
    return bloop_new_generator(bloop_distortion_, v);
}

typedef struct bloop_repeat_data {
    bloop_generator *input;
    int every;
} bloop_repeat_data;

float bloop_repeat_(void *value, int tick) {
    bloop_repeat_data *data = (bloop_repeat_data *) value;
    return bloop_run(data->input, tick % data->every);
}

bloop_generator *bloop_repeat(bloop_generator *input, int every) {
    bloop_repeat_data *v = malloc(sizeof(*v));
    v->input = input;
    v->every = every;
    return bloop_new_generator(bloop_repeat_, v);
}

typedef struct bloop_offset_data {
    bloop_generator *input;
    int offset;
} bloop_offset_data;

float bloop_offset_(void *value, int tick) {
    bloop_offset_data *data = (bloop_offset_data*)value;
    int t = tick - data->offset;
    if (t >= 0) {
        return bloop_run(data->input, t);
    }
    return 0.0;
}

bloop_generator *bloop_offset(bloop_generator *input, int offset) {
    bloop_offset_data *v = malloc(sizeof(*v));
    v->input = input;
    v->offset = offset;
    return bloop_new_generator(bloop_offset_, v);
}

int tick = 0;
bloop_generator *generator;

// the sample callback, running in audio thread
static void stream_cb(float* buffer, int num_frames, int num_channels) {
    static uint32_t count = 0;
    for (int i = 0; i < num_frames; i++) {
        buffer[i] = bloop_run(generator, tick);
        tick += 1;
    }
}

void init(void) {
    saudio_setup(&(saudio_desc){
        .stream_cb = stream_cb
    });
    generator = bloop_sine_wave(LFO(1.0, 440.0, 110.0), C(1.0)); 
    bloop_generator *kick_drum = bloop_distortion(bloop_sine_wave(bloop_interpolation(90, 36, 4000), bloop_adsr(1.0, 0.2, 500, 500, 4000, 2000)), bloop_interpolation(0.9, 0.2, 100), C(1.0));
    generator = bloop_sine_wave(bloop_lfo(LFO(8.0, 24, 24), C(880), C(440.0)), bloop_lfo(C(128.0), C(0.2), LFO(2, 0.1, 0.05)));
    generator = bloop_repeat(kick_drum, 44100);
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });
    pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .value={1.0f, 0.0f, 0.0f, 1.0f} }
    };
}

void frame(void) {
    float g = pass_action.colors[0].value.g + 0.01f;
    pass_action.colors[0].value.g = (g > 1.0f) ? 0.0f : g;
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
    saudio_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 400,
        .height = 300,
        .window_title = "Clear Sample",
    };
}

