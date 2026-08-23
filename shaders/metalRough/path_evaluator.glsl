#ifndef PATH_EVALUATOR_GLSL
#define PATH_EVALUATOR_GLSL

#include "evaluation_options.glsl"
#include "../common/path.glsl"
#include "../common/debug.glsl"

// After the refactor, the per-vertex evaluation happens in the closest-hit
// shader and is folded into path_state.unbiased_light / .biased_light while
// takePathSample streams. The former per-array evaluate loop is gone; these
// accessors just surface the running totals.
//
// eval_options.evaluation_depth is honoured implicitly: raygen passes the
// desired depth to takePathSample so sampling stops there, and the streaming
// reduce covers exactly the sampled vertices.

vec3 evaluatePath(EvaluationOptions options, inout uvec4 rng_state) {
    if (options.debug_path_len) {
        return getDepthDebugColor(path_state.len);
    }
    return path_state.unbiased_light;
}

vec3 evaluatePathBiased(EvaluationOptions options, inout uvec4 rng_state) {
    if (options.debug_path_len) {
        return getDepthDebugColor(path_state.len);
    }
    return path_state.biased_light;
}

#endif
