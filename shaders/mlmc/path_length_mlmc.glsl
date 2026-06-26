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

        takePathSample(biased_path_length);
        color += evaluatePath(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

vec3 pathLengthDiffMlmc(uint sample_count, uint biased_path_length, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);

        takePathSample(unbiased_path_length);

        uvec4 rng = payload.rng_state;

        eval_options.evaluation_depth = biased_path_length;
        vec3 biased_color = evaluatePath(eval_options, payload.rng_state);

        payload.rng_state = rng;

        eval_options.evaluation_depth = unbiased_path_length;
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
