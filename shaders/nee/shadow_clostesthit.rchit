#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "./shadow_payload.glsl"
#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../common/constants.glsl"

#include "../metalRough/vertex_evaluator.glsl"

layout(location = 0) rayPayloadInEXT ShadowPayload payload;

hitAttributeEXT vec3 attribs;

vec3 ratioTracking(vec3 origin, vec3 dir, int volume_idx, inout uvec4 rng_state) {
    float dist_to_boundary = gl_HitTEXT;
    float max_tracking_dist = min(dist_to_boundary, payload.dist_to_light);
    float tracked_dist = 0;

    VolumeInstance volumeInstance = getVolume(volume_idx);
    vec3 transmittance = vec3(1);

    while (true) {
        float sampled_dist = sampleDistance(volumeInstance.majorant, rng_state);
        tracked_dist += sampled_dist;

        if (tracked_dist >= max_tracking_dist) {
            break;
        }

        vec3 curr_pos = origin + tracked_dist * dir;
        vec3 obj_pos = (gl_WorldToObjectEXT * vec4(curr_pos, 1.0f)).xyz;
        EvaluatedVolume volume = evaluateVolumeAtLocalPos(volumeInstance, obj_pos);

        transmittance *= (1.0 - (volume.scattering + volume.absorption) / volume.majorant);
    }

    return transmittance;
}

void main() {
    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    if (!isVolumeBoundary(triangle)) {
        payload.dist_to_light = 0;
        payload.transmittance = vec3(0);
        return;
    }

    VolumeInstance volume = getVolume(triangle);

    vec3 tracking_origin = payload.next_origin;
    bool is_inside_volume = payload.current_volume_idx >= 0;

    payload.next_origin += gl_HitTEXT * payload.direction;
    payload.dist_to_light -= gl_HitTEXT;

    if (is_inside_volume) {
        payload.transmittance *= ratioTracking(tracking_origin, normalize(payload.direction), payload.current_volume_idx, payload.rng_state);
        payload.current_volume_idx = -1;
    } else {
        payload.current_volume_idx = getVolumeIdx(triangle);
    }
}
