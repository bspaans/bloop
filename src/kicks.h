#include "bloop.h"


bloop_generator *bloop_sine_kick_drum();
bloop_generator *bloop_velocity_adjusted_sine_kick_drum(float velocity); // TODO: support velocity generator => new base generator?
bloop_generator *bloop_distorted_sine_kick_drum();
bloop_generator *bloop_kick_drum_rumble(bloop_generator *kick_drum);
