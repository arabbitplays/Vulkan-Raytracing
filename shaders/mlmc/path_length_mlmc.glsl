#ifndef PATH_LENGTH_MLMC_GLSL
#define PATH_LENGTH_MLMC_GLSL

#include "../common/path_sampler.glsl"

vec3 pathLengthMlmc(uint sample_count, uint biased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = biased_path_length;

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        // path_length's biased and unbiased branches share eval parameters;
        // the biased side is just a truncated reduction over the same result
        // stream, so we only fill the unbiased branch and stop sampling early.
        payload.eval_flags = EVAL_FLAG_UNBIASED;
        takePathSample(biased_path_length);
        color += evaluatePath(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

// Prefix correlation: sample one unbiased path up to unbiased_path_length and
// evaluate it twice from the same result stream -- once truncated to
// biased_path_length (via the truncated_light snapshot) and once full-length.
vec3 pathLengthPrefixDiffMlmc(uint sample_count, uint biased_path_length, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.eval_flags = EVAL_FLAG_UNBIASED;
        takePathSample(unbiased_path_length, biased_path_length);
        diff += path_state.unbiased_light - path_state.truncated_light;
    }
    diff /= sample_count;
    return diff;
}

// Resample correlation: draw two independent path samples, one at the
// biased length and one at the unbiased length; no shared random state.
vec3 pathLengthResampleDiffMlmc(uint sample_count, uint biased_path_length, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay biased_view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(biased_view_ray.origin, biased_view_ray.direction);
        payload.eval_flags = EVAL_FLAG_UNBIASED;
        takePathSample(biased_path_length);
        eval_options.evaluation_depth = biased_path_length;
        vec3 biased_color = evaluatePath(eval_options, payload.rng_state);

        ViewRay unbiased_view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(unbiased_view_ray.origin, unbiased_view_ray.direction);
        payload.eval_flags = EVAL_FLAG_UNBIASED;
        takePathSample(unbiased_path_length);
        eval_options.evaluation_depth = unbiased_path_length;
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
