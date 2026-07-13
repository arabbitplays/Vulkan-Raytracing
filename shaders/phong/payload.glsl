#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

struct Payload {
    int depth;
    uvec4 rng_state;
    vec3 next_dir;
    vec3 light;
    vec3 beta;
};

#endif