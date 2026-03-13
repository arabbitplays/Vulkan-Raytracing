#ifndef DISTANCE_SAMPLER_GLSL
#define DISTANCE_SAMPLER_GLSL

#include "../common/random.glsl"

float sampleDistance(float extinction, inout uvec4 rngState) {
    float r = stepAndOutputRNGFloat(rngState);
    float t = -log(1 - r) / extinction;
    return t;
}

float distanceSamplingPdf(float distance, float extinction) {
    return extinction * exp(-distance * extinction);
}

#endif