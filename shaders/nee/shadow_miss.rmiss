#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "./shadow_payload.glsl"

#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"

layout(location = 0) rayPayloadInEXT ShadowPayload payload;

void main() {
    if (payload.current_volume_idx < 0) {
        payload.dist_left = 0;
        // transmittance should already be calculated
    } else {
        float traveled_distance = payload.next_distance;
        vec3 P = payload.next_origin + traveled_distance * payload.direction;

        VolumeInstance volume = getVolume(payload.current_volume_idx);

        vec3 vol_uv = posToVolumeUV(volume, P, payload.volume_world_to_object);
        vec2 coefficients = getCoefficients(volume, vol_uv);

        float absorption = coefficients.x;
        float scattering = coefficients.y;
        float null_scattering = volume.majorant - absorption - scattering;

        payload.transmittance *= null_scattering / volume.majorant;

        payload.next_origin = P;
        payload.next_distance = sampleDistance(volume.majorant, payload.rng_state);
        payload.dist_left -= traveled_distance;
    }
}