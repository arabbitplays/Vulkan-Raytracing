#ifndef ALTERED_PHASE_FUNCTION_GLSL
#define ALTERED_PHASE_FUNCTION_GLSL

#include "../common/math.glsl"
#include "../common/random.glsl"
#include "../volume/phase_function.glsl"

#include "altered_phase_coefficients.glsl"

const float COS_THETA_BIN_WIDTH = 2.0 / float(NUM_COEFFS);

int findBestIndex(float g) {
    int bestIdx = 0;
    float bestDist = abs(g - G_KEYS[0]);

    for (int i = 1; i < NUM_KEYS; ++i)
    {
        float dist = abs(g - G_KEYS[i]);

        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

// (g, alpha) lookup: minimum L1 distance over both keys, used when the
// caller has an explicit alpha (multiple tables can share the same g).
int findBestIndex(float g, float alpha) {
    int bestIdx = 0;
    float bestDist = abs(g - G_KEYS[0]) + abs(alpha - ALPHAS[0]);

    for (int i = 1; i < NUM_KEYS; ++i)
    {
        float dist = abs(g - G_KEYS[i]) + abs(alpha - ALPHAS[i]);

        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

float getSimilarityRelationsAlpha(float g) {
    return ALPHAS[findBestIndex(g)];
}

int cosThetaToBin(float cosTheta) {
    int idx = int((cosTheta + 1.0) * float(NUM_COEFFS) * 0.5);
    return clamp(idx, 0, NUM_COEFFS - 1);
}

float evaluateAlteredPhaseFunction(vec3 wo, vec3 wi, float g, float alpha) {
    int idx = cosThetaToBin(-dot(wo, wi));
    return fetchPhaseCoefficient(findBestIndex(g, alpha), idx);
}

float evaluateAlteredPhaseFunction(vec3 wo, vec3 wi, float g) {
    int idx = findBestIndex(g);
    return evaluateAlteredPhaseFunction(wo, wi, g, ALPHAS[idx]);
}

// Importance-samples the tabulated phase function via CDF inversion.
// wo is the direction toward the previous vertex (e.g. -ray_dir); the
// sampled wi follows the same convention as evaluateAlteredPhaseFunction,
// i.e. the scattering cosine is -dot(wo, wi).
PhaseFunctionSample sampleAlteredPhaseFunction(vec3 wo, float g, float alpha, inout uvec4 rng_state) {
    int tableIdx = findBestIndex(g, alpha);

    float total = 0.0;
    for (int i = 0; i < NUM_COEFFS; ++i) {
        total += fetchPhaseCoefficient(tableIdx, i);
    }

    float u = stepAndOutputRNGFloat(rng_state);
    float target = u * total;

    float accum = 0.0;
    int selectedIdx = NUM_COEFFS - 1;
    float selectedC = fetchPhaseCoefficient(tableIdx, NUM_COEFFS - 1);
    for (int i = 0; i < NUM_COEFFS; ++i) {
        float c = fetchPhaseCoefficient(tableIdx, i);
        if (accum + c > target) {
            selectedIdx = i;
            selectedC = c;
            break;
        }
        accum += c;
    }

    float frac = selectedC > 0.0 ? clamp((target - accum) / selectedC, 0.0, 1.0) : 0.5;
    float cosTheta = clamp(-1.0 + (float(selectedIdx) + frac) * COS_THETA_BIN_WIDTH, -1.0, 1.0);
    float sinTheta = safeSqrt(1.0 - sqr(cosTheta));
    float phi = 2.0 * PI * stepAndOutputRNGFloat(rng_state);

    // Build the frame around -wo so that -dot(wo, wi) == cosTheta.
    Frame wFrame = frameFromZ(normalize(-wo));
    vec3 wi = fromLocal(sphericalDirection(sinTheta, cosTheta, phi), wFrame);

    // CDF-inversion pdf over solid angle: c / (total * dCosTheta * 2*PI).
    float pdf = selectedC / max(total * COS_THETA_BIN_WIDTH * 2.0 * PI, 1e-30);
    return PhaseFunctionSample(selectedC, wi, pdf);
}

PhaseFunctionSample sampleAlteredPhaseFunction(vec3 wo, float g, inout uvec4 rng_state) {
    int idx = findBestIndex(g);
    return sampleAlteredPhaseFunction(wo, g, ALPHAS[idx], rng_state);
}

#endif
