#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

#include "path_vertex.glsl"

struct Payload {
    vec3 light;
    int depth;
    uvec4 rng_state;
    PathVertex next_vertex;
    SampledSegment next_segment;
    vec3 beta;
    float eta_scale; // used for russian roulette
    bool specular_bounce;
};

#endif