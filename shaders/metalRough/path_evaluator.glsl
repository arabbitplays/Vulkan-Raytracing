#ifndef PATH_EVALUATOR_GLSL
#define PATH_EVALUATOR_GLSL

#include "evaluation_options.glsl"
#include "vertex_evaluator.glsl"
#include "../common/path.glsl"
#include "../common/path_vertex.glsl"
#include "../common/debug.glsl"

vec3 evaluateVertex(PathVertex vertex, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    if (vertex.type == SURFACE_TYPE) {
        return evaluateSurfaceVertex(vertex, options, context, rng_state);
    } else if (vertex.type == VOLUME_BOUNDARY_TYPE) {
        return vec3(0);
    } else if (vertex.type == VOLUME_TYPE) {
        return evaluateVolumeVertex(vertex, options, context, rng_state);
    } else if (vertex.type == ENVIRONMENT_TYPE) {
        return vec3(0);
    }
    
    return vec3(1, 0, 0);
}

vec3 evaluatePath(EvaluationOptions options, inout uvec4 rng_state) {
    EvaluationContext context;
    vec3 light = vec3(0);
    vec3 beta = vec3(1);
    context.specular_bounce = false;

    if (options.debug_path_len) {
        return getDepthDebugColor(path.len);
    }

    for (int i = 0; i < min(options.evaluation_depth, path.len); i++) {
        context.depth = i;
        beta *= getPreEvaluationBeta(path.segments[i]);
        light += beta * evaluateVertex(path.vertices[i], options, context, rng_state);
        beta *= getPostEvaluationBeta(path.segments[i]);
    }

    return light;
}

#endif
