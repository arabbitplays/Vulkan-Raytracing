#ifndef PATH_LENGTH_MLMC_GLSL
#define PATH_LENGTH_MLMC_GLSL

#include "../common/path_sample.glsl"

#define SHORT_PATH_LENGTH 5
#define FULL_PATH_LENGTH 20

vec3 pathLengthMlmc(uint sample_count) {
    ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
    initPayload(view_ray.origin, view_ray.direction);

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        color += takeSample(SHORT_PATH_LENGTH);
    }
    color /= sample_count;
    return color;
}

vec3 pathLengthDiffMlmc(uint sample_count) {
    ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
    initPayload(view_ray.origin, view_ray.direction);

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        while (payload.depth < SHORT_PATH_LENGTH && payload.next_direction != vec3(0.0) && payload.next_distance > 0) {
            continuePath();
        }
        vec3 biased_color = payload.light;
        while (payload.depth < FULL_PATH_LENGTH && payload.next_direction != vec3(0.0) && payload.next_distance > 0) {
            continuePath();
        }
        diff += payload.light - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif