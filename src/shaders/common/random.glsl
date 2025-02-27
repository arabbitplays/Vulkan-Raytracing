#include "math.glsl"

layout(binding = 9, set = 0, rgba32ui) uniform uimage2D rng_tex;

// from https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-37-efficient-random-number-generation-and-application
// S1, S2, S3, and M are all constants, and z is part of the
// private per-thread generator state.
uint TausStep(inout uint z, uint S1, uint S2, uint S3, uint M)
{
    uint b = (((z << S1) ^ z) >> S2);
    return z = (((z & M) << S3) ^ b);
}

// A and C are constants
uint LCGStep(inout uint z, uint A, uint C)
{
    return z = (A * z + C);
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uvec4 rngState) {
    return 2.3283064365387e-10 * float(
        TausStep(rngState.x, 13, 19, 12, 4294967294)
        ^ TausStep(rngState.y, 2, 25, 4, 4294967288)
        ^ TausStep(rngState.z, 3, 11, 17, 4294967280)
        ^ LCGStep(rngState.w, 1664525, 1013904223));
}

// ---------------------------------------------------------------------------------------

vec3 sampleUniformSphere(inout uvec4 rngState) {
    vec2 u = vec2(stepAndOutputRNGFloat(rngState), stepAndOutputRNGFloat(rngState));
    float z = 1 - 2 * u.x;
    float r = safeSqrt(1 - z * z);
    float phi = 2 * PI * u.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 sampleUniformHemisphere(inout uvec4 rngState) {
    vec2 u = vec2(stepAndOutputRNGFloat(rngState), stepAndOutputRNGFloat(rngState));

    float z = u.x;
    float r = safeSqrt(1 - z * z);
    float phi = 2 * PI * u.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 sampleCosHemisphere(inout uvec4 rngState) {
    vec2 u = vec2(stepAndOutputRNGFloat(rngState), stepAndOutputRNGFloat(rngState));

    float theta = acos(sqrt(u.x));
    float phi = 2.0 * PI * u.y;

    float x = sin(theta) * cos(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(theta);

    return vec3(x, y, z);
}