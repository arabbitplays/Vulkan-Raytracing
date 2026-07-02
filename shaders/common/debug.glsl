#ifndef DEBUG_GLSL
#define DEBUG_GLSL

#include "luminance.glsl"
#include "adaptive_sampling.glsl"

#define MAX_DEPTH 20
vec3 getDepthDebugColor(uint depth) {
    float t = float(depth) / MAX_DEPTH;
    vec3 blue  = vec3(0.0, 0.0, 1.0);
    vec3 green = vec3(0.0, 1.0, 0.0);
    vec3 red   = vec3(1.0, 0.0, 0.0);
    vec3 c = mix(mix(blue, green, clamp(t * 2.0, 0.0, 1.0)),
               red, clamp(t * 2.0 - 1.0, 0.0, 1.0));
    return c;
}

vec3 getVarianceDebugColor(float second_moment, vec3 mean_color) {
    float lum = luminance(mean_color);
    float variance = max(second_moment - lum * lum, 0.0);
    return vec3(variance);
}

vec3 getAdaptiveSamplingDebugColor(vec3 mean_color, float second_moment, uint sample_count, float error_bound, vec3 fallback) {
    float lum = luminance(mean_color);
    if (stop_sampling(lum, second_moment, sample_count, error_bound)) {
        return vec3(1.0, 0.0, 0.0);
    }
    return fallback;
}

#endif
