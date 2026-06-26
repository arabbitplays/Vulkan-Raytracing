#ifndef PATH_LENGTH_MLMC_GLSL
#define PATH_LENGTH_MLMC_GLSL

#include "../common/path_sampler.glsl"

vec3 pathLengthMlmc(uint sample_count, uint biased_path_length) {
    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);

        color += takeSample(biased_path_length);
    }
    color /= sample_count;
    return color;
}

vec3 pathLengthDiffMlmc(uint sample_count, uint biased_path_length, uint unbiased_path_length) {
    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);

        while (payload.depth < biased_path_length && payload.next_dir != vec3(0.0) && length(payload.beta) > 0) {
            continuePath();
        }
        vec3 biased_color = payload.light;
        while (payload.depth < unbiased_path_length && payload.next_dir != vec3(0.0) && length(payload.beta) > 0) {
            continuePath();
        }
        diff += payload.light - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
