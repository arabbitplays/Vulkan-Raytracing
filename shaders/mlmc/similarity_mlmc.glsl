#ifndef SIMILARITY_MLMC_GLSL
#define SIMILARITY_MLMC_GLSL

#include "../common/path_sampler.glsl"
#include "../common/luminance.glsl"
#include "correlation_modes.glsl"

// The similarity_kind selects which biased mode is active (normal similarity vs. first-order similarity);
void setSamplingBiased(inout SamplingOptions opts, uint similarity_kind, bool biased) {
    if (similarity_kind == SIMILARITY_KIND_FIRST_ORDER) {
        opts.similarity_relation = false;
        opts.use_first_order_similarity = biased;
    } else {
        opts.similarity_relation = biased;
        opts.use_first_order_similarity = false;
    }
}

void setEvalBiased(inout EvaluationOptions opts, uint similarity_kind, bool biased) {
    if (similarity_kind == SIMILARITY_KIND_FIRST_ORDER) {
        opts.use_similarity_relation = false;
        opts.use_first_order_similarity = biased;
    } else {
        opts.use_similarity_relation = biased;
        opts.use_first_order_similarity = false;
    }
}

vec3 similarityMlmc(uint sample_count, uint unbiased_path_length, uint similarity_kind) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;
    setEvalBiased(eval_options, similarity_kind, true);

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        setSamplingBiased(payload.sampling_options, similarity_kind, true);
        if (!mlmcUsesResample()) {
            payload.sampling_options.regular_tracking = true;
        }
        payload.eval_flags = EVAL_FLAG_BIASED;

        takePathSample(unbiased_path_length);
        color += evaluatePathBiased(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

vec3 similarityDiffMlmc(uint sample_count, uint unbiased_path_length, uint correlation_mode, uint similarity_kind) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;

    if (correlation_mode != RESAMPLE_CORRELATION_MODE) {
        vec3 diff = vec3(0);
        for (int i = 0; i < sample_count; i++) {
            ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
            initPayload(view_ray.origin, view_ray.direction);
            setSamplingBiased(payload.sampling_options, similarity_kind, false);
            payload.sampling_options.regular_tracking = true;
            payload.eval_flags = EVAL_FLAG_BIASED | EVAL_FLAG_UNBIASED;
            takePathSample(unbiased_path_length);
            // Streaming reduce handled the shouldSkip / redo logic inline for
            // SKIP_DETERMINISTIC, and applied cross-branch re-derived pre/post
            // beta from the closest hit for SAME_PATH -- both correlated diffs
            // fall out of the same subtraction.
            diff += path_state.unbiased_light - path_state.biased_light;
        }
        return diff / sample_count;
    }

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);

        uvec4 sampling_rng = payload.rng_state;
        initPayload(view_ray.origin, view_ray.direction);
        setSamplingBiased(payload.sampling_options, similarity_kind, false);
        payload.eval_flags = EVAL_FLAG_UNBIASED;
        takePathSample(unbiased_path_length);
        setEvalBiased(eval_options, similarity_kind, false);
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        payload.rng_state = sampling_rng;
        initPayload(view_ray.origin, view_ray.direction);
        setSamplingBiased(payload.sampling_options, similarity_kind, true);
        payload.eval_flags = EVAL_FLAG_BIASED;
        takePathSample(unbiased_path_length);
        setEvalBiased(eval_options, similarity_kind, true);
        vec3 biased_color = evaluatePathBiased(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
