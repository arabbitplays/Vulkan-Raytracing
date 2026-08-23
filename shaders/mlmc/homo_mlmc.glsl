#ifndef HOMO_MLMC_GLSL
#define HOMO_MLMC_GLSL

#include "../common/path_sampler.glsl"
#include "../common/luminance.glsl"
#include "correlation_modes.glsl"

vec3 homoMlmc(uint sample_count, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;
    eval_options.assume_homogenous = true;

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = true;
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

vec3 homoDiffMlmc(uint sample_count, uint unbiased_path_length, uint correlation_mode) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;

    if (correlation_mode != RESAMPLE_CORRELATION_MODE) {
        vec3 diff = vec3(0);
        for (int i = 0; i < sample_count; i++) {
            ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
            initPayload(view_ray.origin, view_ray.direction);
            payload.sampling_options.assume_homogenous = true;
            payload.sampling_options.regular_tracking = true;
            payload.eval_flags = EVAL_FLAG_BIASED | EVAL_FLAG_UNBIASED;
            takePathSample(unbiased_path_length);
            // Streaming reduce already accumulated biased and unbiased totals
            // over the same sampled path; the correlated diff is the pointwise
            // difference of those totals.
            diff += path_state.unbiased_light - path_state.biased_light;
        }
        return diff / sample_count;
    }

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);

        uvec4 sampling_rng = payload.rng_state;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = false;
        payload.eval_flags = EVAL_FLAG_UNBIASED;
        takePathSample(unbiased_path_length);
        eval_options.assume_homogenous = false;
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        payload.rng_state = sampling_rng;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = true;
        payload.eval_flags = EVAL_FLAG_BIASED;
        takePathSample(unbiased_path_length);
        eval_options.assume_homogenous = true;
        vec3 biased_color = evaluatePathBiased(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
