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

vec3 ratioTracking(vec3 origin, vec3 dir, float max_tracking_dist, int volume_idx, inout uvec4 rng_state) {
    float tracked_dist = 0;

    VolumeInstance volumeInstance = getVolume(volume_idx);
    vec3 obj_pos = (gl_WorldToObjectEXT * vec4(origin, 1.0f)).xyz;
    EvaluatedVolume volume = evaluateVolumeAtLocalPos(volumeInstance, payload.similarity_relation, payload.use_first_order_similarity, obj_pos);

    vec3 transmittance = vec3(1);

    while (true) {
        float sampled_dist = sampleDistance(volume.majorant, rng_state);
        tracked_dist += sampled_dist;

        if (tracked_dist >= max_tracking_dist) {
            break;
        }

        vec3 curr_pos = origin + tracked_dist * dir;
        obj_pos = (gl_WorldToObjectEXT * vec4(curr_pos, 1.0f)).xyz;
        volume = evaluateVolumeAtLocalPos(volumeInstance, payload.similarity_relation, payload.use_first_order_similarity, obj_pos);

        transmittance *= (1.0 - (volume.scattering + volume.absorption) / volume.majorant);
    }

    return transmittance;
}

vec3 analyticTransmittance(float dist, int volume_idx) {
    VolumeInstance volumeInstance = getVolume(volume_idx);
    EvaluatedVolume volume = evaluateHomoVolume(volumeInstance, payload.similarity_relation, payload.use_first_order_similarity);
    vec3 extinction = volume.scattering + volume.absorption;
    return exp(-extinction * dist);
}

void main() {
    // bool entering = (gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT);
    bool is_inside_volume = payload.current_volume_idx >= 0;
    vec3 tracking_origin = payload.next_origin;
    vec3 tracking_dir = normalize(payload.direction);
    float dist_to_boundary = gl_HitTEXT;
    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);

    if (dist_to_boundary < payload.dist_to_light - EPSILON) { // it is a real hit
        if (!isVolumeBoundary(triangle)) { // solids block everything
            payload.dist_to_light = 0;
            payload.transmittance = vec3(0);
            return;
        } else {
            if (is_inside_volume) {
                // track to the volume boundary
                if (payload.assume_homogenous) {
                    payload.transmittance *= analyticTransmittance(gl_HitTEXT, payload.current_volume_idx);
                } else {
                    payload.transmittance *= ratioTracking(tracking_origin, tracking_dir, gl_HitTEXT, payload.current_volume_idx, payload.rng_state);
                }
                payload.current_volume_idx = -1;
            } else {
                payload.current_volume_idx = getVolumeIdx(triangle);
            }
            payload.next_origin += gl_HitTEXT * payload.direction;
            payload.dist_to_light -= gl_HitTEXT;
        }
    } else { // false it, basically like a miss
         if (!is_inside_volume) {
             payload.dist_to_light = 0;
             return;
         } else {
             if (payload.assume_homogenous) {
                 payload.transmittance *= analyticTransmittance(payload.dist_to_light, payload.current_volume_idx);
             } else {
                 payload.transmittance *= ratioTracking(tracking_origin, tracking_dir, payload.dist_to_light, payload.current_volume_idx, payload.rng_state);
             }
             payload.dist_to_light = 0;
             return;
         }
    }
}
