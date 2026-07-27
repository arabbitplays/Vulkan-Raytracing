#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

#include "../common/path_vertex.glsl"

struct SamplingOptions {
    bool similarity_relation;
    bool assume_homogenous;
    bool regular_tracking;
};

struct Payload {
    int depth;
    uvec4 rng_state;
    PackedPathVertex next_vertex;
    SampledSegment next_segment;
    vec3 next_dir;
    vec3 rr_beta;
    float eta_scale; // used for russian roulette
    SamplingOptions sampling_options;
};

#endif