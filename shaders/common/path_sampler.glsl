#ifndef PATH_SAMPLE_GLSL
#define PATH_SAMPLE_GLSL

#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "../common/payload.glsl"
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
    payload.light = vec3(0.0);
    payload.depth = 0;
    payload.beta = vec3(1.0);
    payload.eta_scale = 1;
    payload.specular_bounce = false;
    payload.similarity_relation = options.similarity_relation;
}

void addVertexToPath(PathVertex vertex, SampledSegment segment) {
    if (path.len == MAX_PATH_LENGTH)
        return;
    path.vertices[path.len] = vertex;
    path.segments[path.len] = segment;
    path.len++;
}

void continuePath() {
    float tmin = EPSILON;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, payload.next_vertex.P, tmin, payload.next_dir, INFINITY, 0);
    addVertexToPath(payload.next_vertex, payload.next_segment);

    if (payload.next_vertex.type != INVALID_TYPE) {
        payload.depth++;
    }
}

vec3 takeSample(uint max_depth) {
    initPath();
    while (payload.depth < max_depth && payload.next_dir != vec3(0.0) && length(payload.beta) > 0) {
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
