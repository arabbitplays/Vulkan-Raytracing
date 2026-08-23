#ifndef EVALUATION_OPTIONS_GLSL
#define EVALUATION_OPTIONS_GLSL

#include "options.glsl"

struct EvaluationContext {
    int depth;
    bool specular_bounce;
};

struct EvaluationOptions {
    uint evaluation_depth;
    bool sample_light;
    bool sample_bsdf;
    bool use_similarity_relation;
    bool use_first_order_similarity;
    bool assume_homogenous;
    bool debug_path_len;
};

EvaluationOptions getUserOptions() {
    EvaluationOptions eval_options;
    eval_options.evaluation_depth = uint(options.recursion_depth);
    eval_options.sample_light = options.sample_light;
    eval_options.sample_bsdf = SPEC_SAMPLE_BSDF;
    eval_options.use_similarity_relation = options.similarity_relation;
    eval_options.use_first_order_similarity = options.use_first_order_similarity;
    eval_options.assume_homogenous = options.assume_homogenous;
    eval_options.debug_path_len = options.debug_depth;
    return eval_options;
}

#endif
