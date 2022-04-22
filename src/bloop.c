#define SOKOL_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "bloop.h"

bloop_generator* bloop_new_generator(float (*fn)(void*, int), enum bloop_generator_type type, void *userData) {
    bloop_generator *closure = malloc(sizeof(*closure));
    closure->fn = fn;
    closure->type = type;
    closure->userData = userData;
    return closure;
}

#define max(x, y) (x > y ? x : y)

int bloop_generator_depth(bloop_generator *g) {

    bloop_sine_wave_data *sine_d; 
    bloop_white_noise_data *noise_d;
    bloop_lfo_data *lfo_d;
    bloop_distortion_data *dist_d;
    bloop_delay_data *delay_d;
    bloop_repeat_data *repeat_d;
    bloop_offset_data *offset_d;
    bloop_average_data *average_d;

    switch (g->type) {
        case BLOOP_CONSTANT:
            return 1;
        case BLOOP_SINE:
            sine_d = g->userData;
            return 1 + max(bloop_generator_depth(sine_d->pitch), bloop_generator_depth(sine_d->gain));
        case BLOOP_WHITE_NOISE: 
            noise_d = g->userData;
            return 1 + bloop_generator_depth(noise_d->gain);
        case BLOOP_INTERPOLATION: 
            return 1;
        case BLOOP_ADSR:
            return 1;
        case BLOOP_LFO:
            lfo_d = g->userData;
            return 1 + max(max(bloop_generator_depth(lfo_d->speed), 
                               bloop_generator_depth(lfo_d->offset)), 
                               bloop_generator_depth(lfo_d->amount));
        case BLOOP_DISTORTION: 
            dist_d = g->userData;
            return 1 + max(max(bloop_generator_depth(dist_d->input), 
                               bloop_generator_depth(dist_d->level)), 
                               bloop_generator_depth(dist_d->gain));
        case BLOOP_DELAY:
            delay_d = g->userData;
            return 1 + max(max(max(bloop_generator_depth(delay_d->input), 
                                   bloop_generator_depth(delay_d->delay_samples)), 
                                   bloop_generator_depth(delay_d->factor)),
                                   bloop_generator_depth(delay_d->feedback));

        case BLOOP_REPEAT:
            repeat_d = g->userData;
            return 1 + bloop_generator_depth(repeat_d->input);
        case BLOOP_OFFSET:
            offset_d = g->userData;
            return 1 + bloop_generator_depth(offset_d->input);
        case BLOOP_AVERAGE:
            average_d = g->userData;
            int m = 0;
            for (int i = 0; i < average_d->count; i++) {
                int n = bloop_generator_depth(average_d->inputs[i]);
                if (n > m) {
                    m = n;
                }
            }
            return 1 + m;
    }
    return 0;
}

int SAMPLE_RATE = 44100;


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
    return bloop_new_generator(bloop_sine_wave_, BLOOP_SINE, v);
}



float bloop_white_noise_(void *value, int tick) {
    bloop_white_noise_data *data = (bloop_white_noise_data *) value;
    float v = (float)rand()/(float)(RAND_MAX/2.0) - -1;
    return v * bloop_run(data->gain, tick);
}

bloop_generator *bloop_white_noise(bloop_generator *gain) {
    bloop_white_noise_data *v = malloc(sizeof(*v));
    v->gain = gain;
    return bloop_new_generator(bloop_white_noise_, BLOOP_WHITE_NOISE, v);
}



float bloop_constant_(void *value, int tick) {
    float *v = (float *)value;
    return *v;
}

bloop_generator *bloop_constant(float value) {
    float *v = malloc(sizeof(float));
    *v = value;
    return bloop_new_generator(bloop_constant_, BLOOP_CONSTANT, v); 
}



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
    return bloop_new_generator(bloop_interpolation_, BLOOP_INTERPOLATION, v);
}



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
    return bloop_new_generator(bloop_adsr_, BLOOP_ADSR, v);
}




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
    return bloop_new_generator(bloop_lfo_, BLOOP_LFO, v);
}


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
    return bloop_new_generator(bloop_distortion_, BLOOP_DISTORTION, v);
}



float bloop_delay_(void *value, int tick) {
    bloop_delay_data *data = (bloop_delay_data *) value;
    float s = bloop_run(data->input, tick);
    float factor = bloop_run(data->factor, tick);
    float feedback = bloop_run(data->feedback, tick);
    int delay_samples = (int)bloop_run(data->delay_samples, tick);

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
    v->input = input;
    v->delay_samples = delay_samples;
    v->factor = factor;
    v->feedback = feedback;
    v->ring_index = 0;
    v->ring = malloc(sizeof(float) * 8 * SAMPLE_RATE); // allocate 8 seconds 
    return bloop_new_generator(bloop_delay_, BLOOP_DELAY, v);
}



float bloop_repeat_(void *value, int tick) {
    bloop_repeat_data *data = (bloop_repeat_data *) value;
    return bloop_run(data->input, tick % data->every);
}

bloop_generator *bloop_repeat(bloop_generator *input, int every) {
    bloop_repeat_data *v = malloc(sizeof(*v));
    v->input = input;
    v->every = every;
    return bloop_new_generator(bloop_repeat_, BLOOP_REPEAT, v);
}



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
    return bloop_new_generator(bloop_offset_, BLOOP_OFFSET, v);
}



float bloop_average_(void *value, int tick) {
    bloop_average_data *data = (bloop_average_data*)value;
    float s = 0.0;
    for (int i = 0; i < data->count; i++) {
        s += bloop_run(data->inputs[i], tick);
    }
    return s / ((float)data->count);
}

bloop_generator *bloop_average(int count, ...) {
    bloop_average_data *v = malloc(sizeof(*v));
    v->count = count;
    v->inputs = malloc(sizeof(bloop_average_data*) * count);
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        v->inputs[i] = va_arg(args, bloop_generator*);
    }
    return bloop_new_generator(bloop_average_, BLOOP_AVERAGE, v);
}

