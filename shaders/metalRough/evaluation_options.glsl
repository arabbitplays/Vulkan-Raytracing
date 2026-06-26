#ifndef EVALUATION_OPTIONS_GLSL
#define EVALUATION_OPTIONS_GLSL

#include "options.glsl"

struct EvaluationContext {
    int depth;
    bool specular_bounce;
};

struct EvaluationOptions {
    bool sample_light;
    bool sample_bsdf;
    bool use_similarity_relation;
};

EvaluationOptions getUserOptions() {
    EvaluationOptions eval_options;
    eval_options.sample_light = options.sample_light;
    eval_options.sample_bsdf = options.sample_bsdf;
    eval_options.use_similarity_relation = options.similarity_relation;
    return eval_options;
}

#endif
