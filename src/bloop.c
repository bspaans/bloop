#define SOKOL_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "bloop.h"

bloop_generator* bloop_new_generator(float (*fn)(bloop_generator *, void*, int), enum bloop_generator_type type, void *userData) {
    bloop_generator *closure = malloc(sizeof(*closure));
    closure->fn = fn;
    closure->type = type;
    closure->userData = userData;
    closure->input_count = 0;
    for (int i = 0; i < BLOOP_MAX_INPUTS; i++) {
        closure->inputs[i] = NULL;
    }
    return closure;
}

int bloop_generator_depth(bloop_generator *g) {
    int result = 0;
    for (int i = 0; i < g->input_count; i++) {
        if (g->inputs[i] == NULL) {
            continue;
        }
        int n = bloop_generator_depth(g->inputs[i]);
        if (n > result) {
            result = n;
        }
    }
    return result + 1;
}

int SAMPLE_RATE = 44100;


float bloop_sine_wave_(bloop_generator *g, void *value, int tick) {
    bloop_sine_wave_data *data = (bloop_sine_wave_data *) value;
    float pitch = bloop_run_input(g, SINE_WAVE_PITCH, tick);
    float p = fmin(fmax(pitch, 0.0), SAMPLE_RATE/2.0);
    float step_size = (p * 2 * M_PI) / (float) SAMPLE_RATE;
    float result = sin(data->phase) * bloop_run_input(g, SINE_WAVE_GAIN, tick);
    data->phase += step_size;
    return result;
}

bloop_generator *bloop_sine_wave(bloop_generator *pitch, bloop_generator *gain) {
    bloop_sine_wave_data *v = malloc(sizeof(bloop_sine_wave_data));
    bloop_generator *g = bloop_new_generator(bloop_sine_wave_, BLOOP_SINE, v);
    g->input_count = 2;
    g->inputs[SINE_WAVE_PITCH] = pitch;
    g->inputs[SINE_WAVE_GAIN] = gain;
    return g;
}



float bloop_white_noise_(bloop_generator *g, void *value, int tick) {
    float v = (float)rand()/(float)(RAND_MAX/2.0) - -1;
    return v * bloop_run_input(g, WHITE_NOISE_GAIN, tick);
}

bloop_generator *bloop_white_noise(bloop_generator *gain) {
    bloop_generator *g = bloop_new_generator(bloop_white_noise_, BLOOP_WHITE_NOISE, NULL);
    g->input_count = 1;
    g->inputs[WHITE_NOISE_GAIN] = gain;
    return g;
}



float bloop_constant_(bloop_generator *g, void *value, int tick) {
    float *v = (float *)value;
    return *v;
}

bloop_generator *bloop_constant(float value) {
    float *v = malloc(sizeof(float));
    *v = value;
    return bloop_new_generator(bloop_constant_, BLOOP_CONSTANT, v); 
}



float bloop_interpolation_(bloop_generator *g, void *value, int tick) {
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
    return bloop_new_generator(bloop_interpolation_, BLOOP_INTERPOLATION, v);
}



float bloop_adsr_(bloop_generator *g, void *value, int tick) {
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
    return bloop_new_generator(bloop_adsr_, BLOOP_ADSR, v);
}




float bloop_lfo_(bloop_generator *g, void *value, int tick) {
    float speed  = bloop_run_input(g, BLOOP_LFO_SPEED, tick);
    float offset = bloop_run_input(g, BLOOP_LFO_OFFSET, tick);
    float amount = bloop_run_input(g, BLOOP_LFO_AMOUNT, tick);

    float stepSize = (speed * M_PI * 2) / (float) SAMPLE_RATE;
    return sin((float)tick * stepSize) * amount + offset;
}

bloop_generator *bloop_lfo(bloop_generator *speed, bloop_generator *offset, bloop_generator *amount) {
    bloop_generator *g = bloop_new_generator(bloop_lfo_, BLOOP_LFO, NULL);
    g->input_count = 3;
    g->inputs[BLOOP_LFO_SPEED] = speed;
    g->inputs[BLOOP_LFO_OFFSET] = offset;
    g->inputs[BLOOP_LFO_AMOUNT] = amount;
    return g;
}


float bloop_distortion_(bloop_generator *g, void *value, int tick) {
    float s    = bloop_run_input(g, BLOOP_DISTORTION_INPUT, tick);
    float lvl  = bloop_run_input(g, BLOOP_DISTORTION_LEVEL, tick);
    float gain = bloop_run_input(g, BLOOP_DISTORTION_GAIN, tick);
    if (s >= lvl) {
        s = lvl;
    } else if (s <= -1*lvl) {
        s = -1 * lvl;
    }
    return s * gain;
}

bloop_generator *bloop_distortion(bloop_generator *input, bloop_generator *level, bloop_generator *gain) {
    bloop_generator *g = bloop_new_generator(bloop_distortion_, BLOOP_DISTORTION, NULL);
    g->input_count = 3;
    g->inputs[BLOOP_DISTORTION_INPUT] = input;
    g->inputs[BLOOP_DISTORTION_LEVEL] = level;
    g->inputs[BLOOP_DISTORTION_GAIN] = gain;
    return g;
}



float bloop_delay_(bloop_generator *g, void *value, int tick) {
    bloop_delay_data *data = (bloop_delay_data *) value;
    float s           = bloop_run_input(g, BLOOP_DELAY_INPUT, tick);
    float factor      = bloop_run_input(g, BLOOP_DELAY_FACTOR, tick);
    float feedback    = bloop_run_input(g, BLOOP_DELAY_FEEDBACK, tick);
    int delay_samples = (int)bloop_run_input(g, BLOOP_DELAY_SAMPLES, tick);

    int prev_index = (data->ring_index - delay_samples);
    if (prev_index < 0) {
        prev_index = 8 * SAMPLE_RATE + prev_index;
    }
    float prev = data->ring[prev_index];
    data->ring[data->ring_index] = s;
    s += prev * factor;
    data->ring[data->ring_index] += feedback * s;
    data->ring_index = (data->ring_index + 1) % (8 * SAMPLE_RATE);
    return s;
}

bloop_generator *bloop_delay(bloop_generator *input, bloop_generator *delay_samples, bloop_generator *factor, bloop_generator *feedback) {
    bloop_delay_data *v = malloc(sizeof(*v));
    v->ring_index = 0;
    v->ring = malloc(sizeof(float) * 8 * SAMPLE_RATE); // allocate 8 seconds 
    bloop_generator *g = bloop_new_generator(bloop_delay_, BLOOP_DELAY, v);
    g->input_count = 4;
    g->inputs[BLOOP_DELAY_INPUT] = input;
    g->inputs[BLOOP_DELAY_SAMPLES] = delay_samples;
    g->inputs[BLOOP_DELAY_FACTOR] = factor;
    g->inputs[BLOOP_DELAY_FEEDBACK] = feedback;
    return g;
}



float bloop_repeat_(bloop_generator *g, void *value, int tick) {
    bloop_repeat_data *data = (bloop_repeat_data *) value;
    return bloop_run_input(g, BLOOP_REPEAT_INPUT, tick % data->every);
}

bloop_generator *bloop_repeat(bloop_generator *input, int every) {
    bloop_repeat_data *v = malloc(sizeof(*v));
    v->every = every;
    bloop_generator *g = bloop_new_generator(bloop_repeat_, BLOOP_REPEAT, v);
    g->input_count = 1;
    g->inputs[BLOOP_REPEAT_INPUT] = input;
    return g;
}



float bloop_offset_(bloop_generator *g, void *value, int tick) {
    bloop_offset_data *data = (bloop_offset_data*)value;
    int t = tick - data->offset;
    if (t >= 0) {
        return bloop_run_input(g, BLOOP_OFFSET_INPUT, t);
    }
    return 0.0;
}

bloop_generator *bloop_offset(bloop_generator *input, int offset) {
    bloop_offset_data *v = malloc(sizeof(*v));
    v->offset = offset;
    bloop_generator *g = bloop_new_generator(bloop_offset_, BLOOP_OFFSET, v);
    g->input_count = 1;
    g->inputs[BLOOP_OFFSET_INPUT] = input;
    return g;
}



float bloop_average_(bloop_generator *g, void *value, int tick) {
    float s = 0.0;
    for (int i = 0; i < g->input_count; i++) {
        s += bloop_run(g->inputs[i], tick);
    }
    return s / ((float)g->input_count);
}

bloop_generator *bloop_average(int count, ...) {
    bloop_generator *g = bloop_new_generator(bloop_average_, BLOOP_AVERAGE, NULL);
    g->input_count = count;
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        g->inputs[i] = va_arg(args, bloop_generator*);
    }
    return g;
}

