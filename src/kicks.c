#include "kicks.h"

bloop_generator *bloop_kick_drum() {

    bloop_generator *kick_drum_hit = bloop_white_noise(bloop_adsr(0.3, 0.0, 150, 150, 0, 0));
    bloop_generator *kick_drum = bloop_distortion(bloop_sine_wave(bloop_interpolation(90, 36, 4000), bloop_adsr(1.0, 0.2, 500, 500, 4000, 2000)), bloop_interpolation(0.9, 0.2, 100), C(1.0));
    return bloop_average(2, kick_drum, kick_drum_hit);
}

bloop_generator *bloop_kick_drum_rumble(bloop_generator *kick_drum) {
    return bloop_delay(kick_drum, LFO(1.0, 22050, 11025), C(0.8), C(0.1));
}
