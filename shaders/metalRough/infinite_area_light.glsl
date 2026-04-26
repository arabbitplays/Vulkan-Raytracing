#ifndef INFINITE_AREA_LIGHT_GLSL
#define INFINITE_AREA_LIGHT_GLSL
#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "light_sampler.glsl"

vec3 uniformLe() {
    return sceneData.sunlightColor.xyz * sceneData.sunlightDirection.w;
}

LightSample uniformSampleLi(inout uvec4 rng_state) {
    LightSample result;
    result.P = sampleUniformSphere(rng_state);
    result.light = uniformLe();
    result.pdf = 4 * PI;
    return result;
}
#endif // INFINITE_AREA_LIGHT_GLSL
