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
    EvaluatedVolume volume = evaluateVolumeAtLocalPos(volumeInstance, payload.similarity_relation, obj_pos);

    vec3 transmittance = vec3(1);

    while (true) {
        float sampled_dist = sampleDistance(volume.majorant, rng_state);
        tracked_dist += sampled_dist;

        if (tracked_dist >= max_tracking_dist) {
            break;
        }

        vec3 curr_pos = origin + tracked_dist * dir;
        obj_pos = (gl_WorldToObjectEXT * vec4(curr_pos, 1.0f)).xyz;
        volume = evaluateVolumeAtLocalPos(volumeInstance, payload.similarity_relation, obj_pos);

        transmittance *= (1.0 - (volume.scattering + volume.absorption) / volume.majorant);
    }

    return transmittance;
}

vec3 analyticTransmittance(float dist, int volume_idx) {
    VolumeInstance volumeInstance = getVolume(volume_idx);
    EvaluatedVolume volume = evaluateHomoVolume(volumeInstance, payload.similarity_relation);
    vec3 extinction = volume.scattering + volume.absorption;
    return exp(-extinction * dist);
}

// Analytic per-voxel transmittance via DDA. Same grid traversal as the
// distance sampler in metal_rough_closesthit.rchit, but only accumulating
// optical depth (no sampling).
vec3 regularTransmittance(vec3 origin, vec3 dir, float max_tracking_dist, int volume_idx) {
    VolumeInstance volume_instance = getVolume(volume_idx);
    ivec3 res = textureSize(scattering_textures[volume_instance.tex_idx], 0);
    vec3 bb_origin = volume_instance.bounding_box_origin.xyz;
    vec3 bb_extent = volume_instance.bounding_box_extent.xyz;
    vec3 voxel_size = bb_extent / vec3(res);

    vec3 obj_origin = (gl_WorldToObjectEXT * vec4(origin, 1.0)).xyz;
    vec3 obj_dir = mat3(gl_WorldToObjectEXT) * dir;

    vec3 grid_pos = (obj_origin - bb_origin) / voxel_size;
    ivec3 voxel = clamp(ivec3(floor(grid_pos)), ivec3(0), res - ivec3(1));

    ivec3 step_dir;
    vec3 t_delta;
    vec3 next_boundary;
    for (int i = 0; i < 3; ++i) {
        if (obj_dir[i] > 0.0) {
            step_dir[i] = 1;
            t_delta[i] = voxel_size[i] / obj_dir[i];
            float edge = bb_origin[i] + float(voxel[i] + 1) * voxel_size[i];
            next_boundary[i] = (edge - obj_origin[i]) / obj_dir[i];
        } else if (obj_dir[i] < 0.0) {
            step_dir[i] = -1;
            t_delta[i] = -voxel_size[i] / obj_dir[i];
            float edge = bb_origin[i] + float(voxel[i]) * voxel_size[i];
            next_boundary[i] = (edge - obj_origin[i]) / obj_dir[i];
        } else {
            step_dir[i] = 0;
            t_delta[i] = INFINITY;
            next_boundary[i] = INFINITY;
        }
    }

    vec3 optical_depth = vec3(0);
    float tracked_dist = 0.0;
    while (tracked_dist < max_tracking_dist) {
        int axis = 0;
        if (next_boundary.y < next_boundary.x) axis = 1;
        if (next_boundary.z < next_boundary[axis]) axis = 2;

        float seg_end = min(next_boundary[axis], max_tracking_dist);
        float seg_len = max(0.0, seg_end - tracked_dist);

        vec3 voxel_center_obj = bb_origin + (vec3(voxel) + 0.5) * voxel_size;
        EvaluatedVolume volume = evaluateVolumeAtLocalPos(volume_instance, payload.similarity_relation, voxel_center_obj);
        optical_depth += (volume.scattering + volume.absorption) * seg_len;
        tracked_dist = seg_end;

        if (tracked_dist >= max_tracking_dist) break;
        voxel[axis] += step_dir[axis];
        next_boundary[axis] += t_delta[axis];
        if (voxel[axis] < 0 || voxel[axis] >= res[axis]) break;
    }
    return exp(-optical_depth);
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
                } else if (payload.regular_tracking) {
                    payload.transmittance *= regularTransmittance(tracking_origin, tracking_dir, gl_HitTEXT, payload.current_volume_idx);
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
             } else if (payload.regular_tracking) {
                 payload.transmittance *= regularTransmittance(tracking_origin, tracking_dir, payload.dist_to_light, payload.current_volume_idx);
             } else {
                 payload.transmittance *= ratioTracking(tracking_origin, tracking_dir, payload.dist_to_light, payload.current_volume_idx, payload.rng_state);
             }
             payload.dist_to_light = 0;
             return;
         }
    }
}
