#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "light_sampler.glsl"

vec3 uniformLe() {
    return sceneData.sunlightColor.xyz * sceneData.sunlightColor.w;
}

LightSample uniformSampleLi() {
    LightSample result;
    result.P = sampleUniformSphere(payload.rng_state);
    result.light = uniformLe();
    result.pdf = 4 * PI;
    return result;
}