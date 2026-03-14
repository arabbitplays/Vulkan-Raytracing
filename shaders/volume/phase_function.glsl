#ifndef PHASE_FUNCTION_GLSL
#define PHASE_FUNCTION_GLSL

#include "../common/math.glsl"
#include "../common/random.glsl"

struct PhaseFunctionSample {
    float p;
    vec3 wi;
    float pdf;
};

float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0 + g2 + 2 * g * cosTheta;
    return INV_4_PI * (1.0 - g2) / (denom * safeSqrt(denom));
}

float henyeyGreenstein(vec3 wo, vec3 wi, float g) {
    return henyeyGreenstein(dot(wo, wi), g);
}

vec3 sampleHenyeyGreenstein(vec3 wo, float g, uvec4 rng_state, inout float pdf) {
    vec2 u = vec2(stepAndOutputRNGFloat(rng_state), stepAndOutputRNGFloat(rng_state));

    float g2 = sqr(g);

    float cosTheta;
    if (abs(g) < 1e-3f)
        cosTheta = 1 - 2 * u.x;
    else
        cosTheta = -1.0 / (2.0 * g) * (1.0 + g2 - sqr((1.0 - g2) / (1.0 + g - 2 * g * u.x)));

    float sinTheta = safeSqrt(1.0 - sqr(cosTheta));
    float phi = 2.0 * PI * u.y;
    Frame wFrame = frameFromZ(wo);
    vec3 wi = fromLocal(sphericalDirection(sinTheta, cosTheta, phi), wFrame);

    pdf = henyeyGreenstein(cosTheta, g);
    return wi;
}

PhaseFunctionSample sampleHGPhaseFunction(vec3 wo, float g, uvec4 rng_state) {
    float pdf;
    vec3 wi = sampleHenyeyGreenstein(wo, g, rng_state, pdf);
    return PhaseFunctionSample(pdf, wi, pdf);
}

PhaseFunctionSample sampleIsoPhaseFunction(vec3 wo, uvec4 rng_state) {
    float pdf;
    vec3 wi = sampleUniformSphere(rng_state);
    return PhaseFunctionSample(INV_4_PI, wi, INV_4_PI);
}

#endif