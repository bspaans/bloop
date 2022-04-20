#include "bloop.h"
#include "ui.h"
#define SOKOL_IMPL
#include <sokol_audio.h>
#include <stdio.h>
#include <math.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#define NK_IMPLEMENTATION
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "util/sokol_nuklear.h"

sg_pass_action pass_action;


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
    generator = bloop_sine_wave(LFO(1.0, 440.0, 110.0), C(1.0)); 
    bloop_generator *kick_drum_hit = bloop_white_noise(bloop_adsr(0.3, 0.0, 150, 150, 0, 0));
    bloop_generator *kick_drum = bloop_distortion(bloop_sine_wave(bloop_interpolation(90, 36, 4000), bloop_adsr(1.0, 0.2, 500, 500, 4000, 2000)), bloop_interpolation(0.9, 0.2, 100), C(1.0));
    bloop_generator *kick_drum1 = bloop_average(2, kick_drum, kick_drum_hit);
    bloop_generator *kick_drum_rumble1 = bloop_delay(kick_drum1, LFO(1.0, 22050, 11025), C(0.8), C(0.1));
    bloop_generator *kick_drum_rumble2 = bloop_repeat(bloop_distortion(bloop_delay(bloop_average(2, kick_drum, kick_drum_rumble1), LFO(1.0, 11025, 5000), LFO(32.0, 0.7, 0.2), LFO(32.0, 0.5, 0.4)), C(0.8), C(4.5)), 88200);
    bloop_generator *wobble2 = bloop_sine_wave(bloop_lfo(LFO(8.0, 24, 24), C(880), C(440.0)), bloop_lfo(C(128.0), C(0.2), LFO(2, 0.1, 0.05)));
    generator = bloop_average(2, kick_drum_rumble2, wobble2);

    saudio_setup(&(saudio_desc){
        .stream_cb = stream_cb
    });
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });
    snk_setup(&(snk_desc_t){

    });
    pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .value={1.0f, 1.0f, 1.0f, 1.0f} }
    };
}

void frame(void) {
    //float g = pass_action.colors[0].value.g + 0.01f;
    //pass_action.colors[0].value.g = (g > 1.0f) ? 0.0f : g;
    struct nk_context *ctx = snk_new_frame();
    node_editor(ctx);
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    snk_render(sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
}

void event_handler(const struct sapp_event *event) {
    snk_handle_event(event);
    switch (event->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE: 
            break;
        default:
            break;
    }
    // TODO sapp_consume_event?
}

void cleanup(void) {
    snk_shutdown();
    saudio_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event_handler,
        .width = 800,
        .height = 600,
        .window_title = "bloop",
    };
}

