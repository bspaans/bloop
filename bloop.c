#define SOKOL_IMPL
#include <sokol_audio.h>
#include <stdio.h>
#include <math.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"

sg_pass_action pass_action;


#define floatGenerator float(int);

int SAMPLE_RATE = 44100;

static float SineWaveGenerator(float pitch, float gain, int tick) {
    float step_size = (pitch * 2 * M_PI) / (float) SAMPLE_RATE;
    return sin((float) tick * step_size);
}


int tick = 0;

// the sample callback, running in audio thread
static void stream_cb(float* buffer, int num_frames, int num_channels) {
    static uint32_t count = 0;
    for (int i = 0; i < num_frames; i++) {
        buffer[i] = SineWaveGenerator(440.0, 1.0, tick);
        tick += 1;
    }
}

void init(void) {
    saudio_setup(&(saudio_desc){
        .stream_cb = stream_cb
    });
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
