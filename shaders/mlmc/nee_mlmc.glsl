#ifndef NEE_MLMC_GLSL
#define NEE_MLMC_GLSL

#include "../common/path_sample.glsl"

vec3 neeMlmc(uint sample_count, uint unbiased_path_length) {
    ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
    initPayload(view_ray.origin, view_ray.direction);

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        color += takeSample(unbiased_path_length);
    }
    color /= sample_count;
    return color;
}

vec3 neeDiffMlmc(uint sample_count, uint unbiased_path_length) {
    ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
    initPayload(view_ray.origin, view_ray.direction);

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        vec3 biased_color;
        while (payload.depth < unbiased_path_length && payload.next_direction != vec3(0.0) && payload.next_distance > 0) {
            continuePath();
        }
        diff += payload.light - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
