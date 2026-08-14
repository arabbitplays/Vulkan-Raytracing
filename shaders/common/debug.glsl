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

// m2 is Welford's sum of squared deviations
vec3 getVarianceDebugColor(float m2, uint sample_count) {
    if (sample_count < 2u) return vec3(0.0);
    float variance = m2 / float(sample_count);
    return vec3(variance);
}

vec3 getAdaptiveSamplingDebugColor(vec3 mean_color, float second_moment, uint sample_count,
                                   uint min_samples, float rel_error, float abs_floor, vec3 fallback) {
    float lum = luminance(mean_color);
    if (stop_sampling(lum, second_moment, sample_count, min_samples, rel_error, abs_floor)) {
        return vec3(1.0, 0.0, 0.0);
    }
    return fallback;
}

// Visualizes half_width / threshold from stop_sampling. Boundary (ratio == 1) is
// yellow; ratio << 1 (deep converged) trends to blue; ratio >> 1 trends to red.
// Log2-scaled so a factor-of-16 change spans the full ramp.
vec3 getConvergenceRatioDebugColor(vec3 mean_color, float m2, uint sample_count,
                                   uint min_samples, float rel_error, float abs_floor) {
    if (sample_count < 2u) return vec3(0.1);
    if (sample_count < min_samples) return vec3(0.3);

    float n = float(sample_count);
    float half_width = ADAPTIVE_SAMPLING_Z * sqrt(m2 / (n * (n - 1.0)));
    float threshold = max(rel_error * abs(luminance(mean_color)), abs_floor);
    float ratio = half_width / max(threshold, 1e-20);

    float t = clamp(log2(max(ratio, 1e-8)) * 0.25 + 0.5, 0.0, 1.0);
    if (t < 0.333) {
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), t / 0.333);
    } else if (t < 0.667) {
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t - 0.333) / 0.334);
    }
    return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t - 0.667) / 0.333);
}

#endif
