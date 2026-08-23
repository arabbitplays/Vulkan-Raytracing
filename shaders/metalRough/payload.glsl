#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

#include "../common/path_vertex.glsl"

struct SamplingOptions {
    bool similarity_relation;
    bool use_first_order_similarity;
    bool assume_homogenous;
    bool regular_tracking;
};

// Bit flags telling the closest-hit shader which per-vertex evaluation branches
// to fill into payload.next_result. Set by the raygen MLMC helpers before each
// takePathSample call so a single sampled path can drive either the biased-only,
// unbiased-only, or both-branches accumulators without extra pipeline variants.
const uint EVAL_FLAG_BIASED   = 0x1u;
const uint EVAL_FLAG_UNBIASED = 0x2u;

struct Payload {
    int depth;
    uvec4 rng_state;
    // eval_rng_state advances only during on-vertex NEE inside the closest-hit;
    // keeping it out of rng_state means sampling of subsequent path vertices is
    // untouched by evaluation and remains identical across correlated modes.
    uvec4 eval_rng_state;
    PackedPathVertex next_vertex;
    SampledSegment next_segment;
    VertexResult next_result;
    uint eval_flags;
    vec3 next_dir;
    vec3 rr_beta;
    float eta_scale; // used for russian roulette
    SamplingOptions sampling_options;
};

#endif