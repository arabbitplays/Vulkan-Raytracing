#ifndef PATH_SAMPLE_GLSL
#define PATH_SAMPLE_GLSL

#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "payload.glsl"
#include "../common/debug.glsl"
#include "../common/path.glsl"
#include "./options.glsl"
#include "path_evaluator.glsl"

layout(location = 0) rayPayloadEXT Payload payload;

void initPayload(vec3 origin, vec3 direction) {
    payload.next_vertex.P = origin;
    payload.next_vertex.volume_idx = -1;
    payload.next_segment.pre_eval_beta = vec3(1);
    payload.next_segment.post_eval_beta = vec3(1);
    payload.next_dir = direction;
    payload.depth = 0;
    payload.rr_beta = vec3(1.0);
    payload.eta_scale = 1;
    SamplingOptions sample_options;
    sample_options.similarity_relation = options.similarity_relation;
    payload.sampling_options = sample_options;
}

void addVertexToPath(PathVertex vertex, SampledSegment segment) {
    if (path.len == MAX_PATH_LENGTH)
        return;
    path.vertices[path.len] = vertex;
    path.segments[path.len] = segment;
    path.len++;
}

bool evalRussianRoulette(vec3 beta, float eta_scale, out float pdf, inout uvec4 rng_state) {
    pdf = 1.0;
    vec3 rr_beta = beta * eta_scale;
    float beta_max_component = max(rr_beta.x, max(rr_beta.y, rr_beta.z));
    if (beta_max_component < 1) {
        float q = max(0, 1 - beta_max_component);
        float u = stepAndOutputRNGFloat(rng_state);
        if (u < q) {
            return true;
        }
        pdf = 1 - q;
    }
    return false;
}

void continuePath() {
    float tmin = EPSILON;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, payload.next_vertex.P, tmin, payload.next_dir, INFINITY, 0);

    payload.rr_beta *= payload.next_segment.pre_eval_beta;
    float rr_pdf = 1;
    bool apply_russian_roulette = options.russian_roulette
        && payload.depth > 1
        && (payload.next_vertex.type == SURFACE_TYPE || payload.next_vertex.type == VOLUME_TYPE);
    if (apply_russian_roulette) {
        if (evalRussianRoulette(payload.rr_beta, payload.eta_scale, rr_pdf, payload.rng_state)) {
            payload.rr_beta = vec3(0);
        } else {
            payload.next_segment.post_eval_beta /= rr_pdf;
        }
    }
    payload.rr_beta *= payload.next_segment.post_eval_beta;

    addVertexToPath(payload.next_vertex, payload.next_segment);

    if (payload.next_vertex.type != INVALID_TYPE) {
        payload.depth++;
    }

}

vec3 takeSample(uint max_depth) {
    initPath();
    while (payload.depth < max_depth && payload.next_dir != vec3(0.0) && length(payload.rr_beta) > 0) {
        continuePath();
    }

    if (options.debug_depth) {
        return getDepthDebugColor(payload.depth);
    }
    //return payload.light;
    EvaluationOptions options = getUserOptions();
    return evaluatePath(options, payload.rng_state);
}


#endif
