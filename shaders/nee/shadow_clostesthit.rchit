#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable


layout(binding = 1, set = 1) uniform sampler2D material_textures[64];

#include "./shadow_payload.glsl"
#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../common/constants.glsl"

layout(location = 0) rayPayloadInEXT ShadowPayload payload;

hitAttributeEXT vec3 attribs;

void main() {
    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    if (isVolumeBoundary(triangle)) {
         VolumeInstance volume = getVolume(triangle);

         payload.next_origin += gl_HitTEXT * payload.direction;
         payload.dist_left -= gl_HitTEXT;

         if (payload.current_volume_idx >= 0) {
             payload.current_volume_idx = -1;
             payload.next_distance = payload.dist_left;
         } else {
             payload.current_volume_idx = getVolumeIdx(triangle);
             payload.volume_world_to_object = gl_WorldToObjectEXT;
             payload.next_distance = sampleDistance(volume.majorant, payload.rng_state);
             // TODO check if you need a pdf here again
        }
    } else {
        payload.dist_left = 0;
        payload.transmittance = vec3(0);
    }
}
