#ifndef BLOOP
#define BLOOP

/* 
 * Bloop is built around the concept of functions that generate floating point
 * numbers for a given tick.  The tick is simply a counter that denotes what
 * sample we're on; e.g. if we want to get the tenth sample we have a tick of 9
 * (because we start counting on 0).
 *
 * To make these functions interesting they often want access to some kind of
 * data or state; this is where the bloop_generator comes in: it keeps track of
 * some user data and has a pointer to the function that is supposed to do the
 * interesting work. If you are familiar with closures, then you can think of
 * this as a poor man's closure. 
 *
 * All the things in bloop are bloop_generators: sine waves, filters, triggers,
 * etc., and some bloop_generators will be able to use other bloop_generators
 * for their parameters, which is where things get interesting and composable.
 *
 * Every bloop_generator usually has an accompanying struct (e.g.
 * bloop_sine_wave_data), which is meant to capture the user data, an
 * implementation function (e.g. bloop_sine_wave_) that takes this struct and a
 * tick, and produces a float, and finally a convenience function (e.g.
 * bloop_sine_wave) that creates the generator.
 *
 * To execute a bloop_generator to get its value, the bloop_run macro can be used.
 */

enum bloop_generator_type {
    BLOOP_SINE,
    BLOOP_WHITE_NOISE,
    BLOOP_INTERPOLATION,
    BLOOP_CONSTANT,
    BLOOP_ADSR,
    BLOOP_LFO,
    BLOOP_DISTORTION,
    BLOOP_DELAY,
    BLOOP_REPEAT,
    BLOOP_OFFSET,
    BLOOP_AVERAGE,
};
#define BLOOP_MAX_INPUTS 8
typedef struct bloop_generator{
    float (*fn)(struct bloop_generator *, void*, int);
    enum bloop_generator_type type;
    void *userData;

    int input_count;
    struct bloop_generator *inputs[BLOOP_MAX_INPUTS];
} bloop_generator;

bloop_generator* bloop_new_generator(float (*fn)(bloop_generator *, void*, int), enum bloop_generator_type type, void *userData);
int bloop_generator_depth(bloop_generator *g);

#define bloop_run(closure, tick) ((*closure->fn)(closure, closure->userData, tick))
#define bloop_run_input(g, input, tick) (bloop_run(g->inputs[input], tick))

#define SINE_WAVE_PITCH 0
#define SINE_WAVE_GAIN  1

typedef struct bloop_sine_wave_data {
    float phase;
} bloop_sine_wave_data;

#define WHITE_NOISE_GAIN 0

typedef struct bloop_interpolation_data {
    float from;
    float to;
    int over;
} bloop_interpolation_data;

typedef struct bloop_adsr_data {
    float max_gain;
    float sustain;

    int attack_samples;
    int decay_samples;
    int sustain_samples;
    int release_samples;
} bloop_adsr_data;

#define BLOOP_LFO_SPEED 0
#define BLOOP_LFO_OFFSET 1
#define BLOOP_LFO_AMOUNT 2

#define BLOOP_DISTORTION_INPUT 0 
#define BLOOP_DISTORTION_LEVEL 1
#define BLOOP_DISTORTION_GAIN 2 

#define BLOOP_DELAY_INPUT 0
#define BLOOP_DELAY_SAMPLES 1
#define BLOOP_DELAY_FACTOR 2
#define BLOOP_DELAY_FEEDBACK 3

typedef struct bloop_delay_data {
    int ring_index;
    float *ring;
} bloop_delay_data;

#define BLOOP_REPEAT_INPUT 0

typedef struct bloop_repeat_data {
    int every;
} bloop_repeat_data;

#define BLOOP_OFFSET_INPUT 0

typedef struct bloop_offset_data {
    int offset;
} bloop_offset_data;

float bloop_sine_wave_(bloop_generator *g, void *value, int tick);
float bloop_white_noise_(bloop_generator *g, void *value, int tick);
float bloop_constant_(bloop_generator *g, void *value, int tick);
float bloop_interpolation_(bloop_generator *g, void *value, int tick);
float bloop_adsr_(bloop_generator *g, void *value, int tick);
float bloop_lfo_(bloop_generator *g, void *value, int tick);
float bloop_distortion_(bloop_generator *g, void *value, int tick);
float bloop_delay_(bloop_generator *g, void *value, int tick);
float bloop_repeat_(bloop_generator *g, void *value, int tick);
float bloop_offset_(bloop_generator *g, void *value, int tick);
float bloop_average_(bloop_generator *g, void *value, int tick);

bloop_generator *bloop_sine_wave(bloop_generator *pitch, bloop_generator *gain);
bloop_generator *bloop_white_noise(bloop_generator *gain);
bloop_generator *bloop_constant(float value);
bloop_generator *bloop_interpolation(float from, float to, int over);
bloop_generator *bloop_adsr(float max_gain, float sustain, int attack_samples, int decay_samples, int sustain_samples, int release_samples);
bloop_generator *bloop_lfo(bloop_generator *speed, bloop_generator *offset, bloop_generator *amount);
bloop_generator *bloop_distortion(bloop_generator *input, bloop_generator *level, bloop_generator *gain);
bloop_generator *bloop_delay(bloop_generator *input, bloop_generator *delay_samples, bloop_generator *factor, bloop_generator *feedback);
bloop_generator *bloop_repeat(bloop_generator *input, int every);
bloop_generator *bloop_offset(bloop_generator *input, int offset);
bloop_generator *bloop_average(int count, ...);

#define C(c) (bloop_constant(c))
#define LFO(speed, offset, amount) (bloop_lfo(bloop_constant(speed), bloop_constant(offset), bloop_constant(amount)))

#endif
