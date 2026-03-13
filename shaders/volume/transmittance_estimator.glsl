#ifndef TRANSMITTANCE_ESTIMATOR_GLSL
#define TRANSMITTANCE_ESTIMATOR_GLSL

float transmittance(float distance, float extinction) {
    return exp(-extinction * distance);
}

#endif