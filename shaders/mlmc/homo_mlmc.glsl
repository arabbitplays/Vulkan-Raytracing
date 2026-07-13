#ifndef HOMO_MLMC_GLSL
#define HOMO_MLMC_GLSL

#include "../common/path_sampler.glsl"
#include "../common/luminance.glsl"

vec3 homoMlmc(uint sample_count, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;
    eval_options.assume_homogenous = true;

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = true;

        takePathSample(unbiased_path_length);
        color += evaluatePath(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

vec3 homoDiffMlmc(uint sample_count, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);

        uvec4 sampling_rng = payload.rng_state;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = false;
        takePathSample(unbiased_path_length);

        uvec4 eval_rng = payload.rng_state;
        eval_options.assume_homogenous = false;
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        payload.rng_state = sampling_rng;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = true;
        takePathSample(unbiased_path_length);

        payload.rng_state = eval_rng;
        eval_options.assume_homogenous = true;
        vec3 biased_color = evaluatePath(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
