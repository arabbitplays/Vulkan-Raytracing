#ifndef PATH_SAMPLE_GLSL
#define PATH_SAMPLE_GLSL

#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "payload.glsl"
#include "../common/path.glsl"
#include "./options.glsl"
#include "path_evaluator.glsl"

layout(location = 0) rayPayloadEXT Payload payload;

void initPayload(vec3 origin, vec3 direction) {
    payload.next_vertex.P = origin;
    payload.next_vertex.meta = packVertexMeta(0u, -1, INVALID_TYPE, false);
    payload.next_dir = direction;
    payload.depth = 0;
    payload.rr_beta = vec3(1.0);
    payload.eta_scale = 1;
    SamplingOptions sample_options;
    sample_options.similarity_relation = options.similarity_relation;
    sample_options.assume_homogenous = options.assume_homogenous;
    sample_options.regular_tracking = options.regular_tracking;
    payload.sampling_options = sample_options;
}

void addVertexToPath(PackedPathVertex vertex, SampledSegment segment) {
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

    payload.rr_beta *= getPreEvaluationBeta(payload.next_segment);
    float rr_pdf = 1;
    int next_vertex_type = getVertexType(payload.next_vertex);
    bool apply_russian_roulette = SPEC_RUSSIAN_ROULETTE
        && payload.depth > 1
        && (next_vertex_type == SURFACE_TYPE || next_vertex_type == VOLUME_TYPE);
    if (apply_russian_roulette) {
        if (evalRussianRoulette(payload.rr_beta, payload.eta_scale, rr_pdf, payload.rng_state)) {
            payload.rr_beta = vec3(0);
        } else {
            payload.next_segment.rr_pdf *= rr_pdf;
        }
    }
    payload.rr_beta *= getPostEvaluationBeta(payload.next_segment);

    addVertexToPath(payload.next_vertex, payload.next_segment);

    if (next_vertex_type != INVALID_TYPE) {
        payload.depth++;
    }

}

void takePathSample(uint max_depth) {
    // vertices beyond MAX_PATH_LENGTH would be dropped by addVertexToPath anyway
    max_depth = min(max_depth, MAX_PATH_LENGTH);
    initPath(payload.next_vertex.P);
    while (payload.depth < max_depth && payload.next_dir != vec3(0.0) && dot(payload.rr_beta, payload.rr_beta) > 0.0) {
        continuePath();
    }
}


#endif
