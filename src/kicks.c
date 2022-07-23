#include "kicks.h"

bloop_generator *bloop_sine_kick_drum() {
    return bloop_interpolated_sine_wave(90, 16, 8000, 
        bloop_adsr(1.0, 0.2, 500, 500, 4000, 2000)
    );
}

bloop_generator *bloop_velocity_adjusted_sine_kick_drum(float velocity) {
    int interpolation_over = 8000 + (int)(8000.0 * velocity);
    return bloop_interpolated_sine_wave(90, 16, interpolation_over, 
        bloop_adsr(velocity, velocity / 5, 
            500 - (int)(50.0 * velocity),  // attack
            500 + (int)(100.0*velocity),   // delay
            4000 + (int)(2000.0 *velocity), // sustain
            2000 + (int)(2000.0 * velocity)) // release
    );
}

bloop_generator *bloop_distorted_sine_kick_drum() {
    return bloop_distortion(
            bloop_sine_kick_drum(),
            bloop_interpolation(0.9, 0.2, 100), 
            C(1.0)
            );
}

bloop_generator *bloop_kick_drum_hit() {
    return bloop_white_noise(bloop_adsr(0.3, 0.0, 150, 150, 0, 0));
}

bloop_generator *bloop_kick_drum_rumble(bloop_generator *kick_drum) {
    return bloop_delay(kick_drum, LFO(1.0, 22050, 11025), C(0.8), C(0.1));
}
