#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/random.glsl"

#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../volume/transmittance_estimator.glsl"

#include "options.glsl"

void main() {
    if (payload.current_volume_idx >= 0) {
        float traveled_distance = payload.next_distance;
        vec3 P = payload.next_origin + traveled_distance * normalize(payload.next_direction);

        VolumeInstance volume = getVolume(payload.current_volume_idx);
        float extinction = volume.scattering + volume.absorption;

        payload.beta *= volume.scattering * transmittance(traveled_distance, extinction) / distanceSamplingPdf(traveled_distance, extinction) * 4.0 * PI;

        payload.next_origin = P;
        payload.next_distance = sampleDistance(extinction, payload.rng_state);
        payload.next_direction = sampleUniformSphere(payload.rng_state);
    } else {
        payload.next_direction = vec3(0);
        if (!options.sample_light || payload.specular_bounce || (payload.depth == 0 && sceneData.sunlightColor.w > 0)) {
            //payload.light += payload.beta * uniformLe();
        }
    }

}