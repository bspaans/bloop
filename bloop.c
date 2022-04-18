#define SOKOL_IMPL
#include <sokol_audio.h>
#include <stdio.h>
#include <math.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"

sg_pass_action pass_action;


typedef float (*floatGenerator)(int);
typedef struct floatGeneratorClosure{
    float (*fn)(void*, int);
    void *userData;
} floatGeneratorClosure;

floatGeneratorClosure* NewFloatGeneratorClosure(float (*fn)(void*, int), void *userData) {
    floatGeneratorClosure *closure = malloc(sizeof(*closure));
    closure->fn = fn;
    closure->userData = userData;
    return closure;
}

float RunClosure(floatGeneratorClosure *closure, int tick) {
    return (*closure->fn)(closure->userData, tick);
}

int SAMPLE_RATE = 44100;

typedef struct SineWaveData {
    floatGeneratorClosure *pitch;
    floatGeneratorClosure *gain;
    float phase;
} SineWaveData;

float SineWaveGenerator_(void *value, int tick) {
    SineWaveData *data = (SineWaveData *) value;
    float pitch = RunClosure(data->pitch, tick);
    float p = fmin(fmax(pitch, 0.0), SAMPLE_RATE/2.0);
    float step_size = (p * 2 * M_PI) / (float) SAMPLE_RATE;
    float result = sin(data->phase) * RunClosure(data->gain, tick);
    data->phase += step_size;
    return result;
}

floatGeneratorClosure *SineWaveGenerator(floatGeneratorClosure *pitch, floatGeneratorClosure *gain) {
    SineWaveData *v = malloc(sizeof(SineWaveData));
    v->pitch = pitch;
    v->gain = gain;
    return NewFloatGeneratorClosure(SineWaveGenerator_, v);
}

float ConstantGenerator_(void *value, int tick) {
    return 440.0;
}

floatGeneratorClosure *ConstantGenerator(float value) {
    float *v = malloc(sizeof(float));
    *v = value;
    return NewFloatGeneratorClosure(ConstantGenerator_, v); 
}

typedef struct LFOData {
    float speed;
    float offset;
    float amount;
} LFOData;

float LFOGenerator_(void *value, int tick) {
    LFOData *data = (LFOData*)value;
	float stepSize = (data->speed * M_PI * 2) / (float) SAMPLE_RATE;
    return sin((float)tick * stepSize) * data->amount + data->offset;
}

floatGeneratorClosure *LFOGenerator(float speed, float offset, float amount) {
    LFOData *v = malloc(sizeof(LFOData));
    v->speed = speed;
    v->offset = offset;
    v->amount = amount;
    return NewFloatGeneratorClosure(LFOGenerator_, v);
}


int tick = 0;
floatGeneratorClosure *generator;

// the sample callback, running in audio thread
static void stream_cb(float* buffer, int num_frames, int num_channels) {
    static uint32_t count = 0;
    for (int i = 0; i < num_frames; i++) {
        buffer[i] = RunClosure(generator, tick);
        tick += 1;
    }
}

void init(void) {
    saudio_setup(&(saudio_desc){
        .stream_cb = stream_cb
    });
    generator = SineWaveGenerator(LFOGenerator(2.0, 440.0, 110.0), ConstantGenerator(1.0)); 
    //generator = SineWaveGenerator(ConstantGenerator(440.0), ConstantGenerator(1.0));
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
