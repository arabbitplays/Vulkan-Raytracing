#ifndef SIMILARITY_MLMC_GLSL
#define SIMILARITY_MLMC_GLSL

#include "../common/path_sampler.glsl"

vec3 similarityMlmc(uint sample_count, uint unbiased_path_length) {
    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.similarity_relation = true;

        color += takeSample(unbiased_path_length);
    }
    color /= sample_count;
    return color;
}

vec3 similarityDiffMlmc(uint sample_count, uint unbiased_path_length) {
    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);

        uvec4 rng = payload.rng_state;
        initPayload(view_ray.origin, view_ray.direction);
        payload.similarity_relation = true;
        vec3 biased_color = takeSample(unbiased_path_length);

        payload.rng_state = rng;
        initPayload(view_ray.origin, view_ray.direction);
        payload.similarity_relation = false;
        vec3 unbiased_color = takeSample(unbiased_path_length);
        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
