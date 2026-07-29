#ifndef VERTEX_EVALUATOR_GLSL
#define VERTEX_EVALUATOR_GLSL

#include "../common/path_vertex.glsl"
#include "../common/scene_data.glsl"

#include "options.glsl"
#include "material.glsl"
#include "light_sampler.glsl"
#include "bsdf_sampler.glsl"
#include "evaluation_options.glsl"

#include "../volume/layout.glsl"
#include "../volume/phase_function.glsl"
#include "../volume/distance_sampler.glsl"
#include "../similarity/altered_phase_function.glsl"
#include "../volume/transmittance_estimator.glsl"

mat3 getTBN(vec3 geom_N, vec3 T) {
    vec3 bitangent = -normalize(cross(geom_N, T));
    return mat3(T, bitangent, geom_N);
}

EvaluatedMaterial evaluateVertexMaterial(PathVertex vertex) {
    EvaluatedMaterial result;
    Material material = getMaterial(vertex.material_idx);
    result.emission_color = material.emission_color;
    result.emission_power = material.emission_power;
    result.albedo = texture(material_textures[material.albedo_tex_idx], vertex.uv).xyz + material.albedo;
    vec3 metal_rough_ao = texture(material_textures[material.metal_rough_ao_tex_idx], vertex.uv).xyz;
    result.metallic = metal_rough_ao.x + material.metallic;
    result.roughness = metal_rough_ao.y + material.roughness;
    result.ao = metal_rough_ao.z + material.ao;
    result.eta = material.eta;
    return result;
}

// First-order similarity is a special case of the general similarity relation
// with alpha = 1 - g and an isotropic phase function. It takes precedence over
// `use_similarity_relation` when both flags are set (host guards against that).
float similarityAlpha(VolumeInstance volume, bool use_similarity_relation, bool use_first_order_similarity) {
    if (use_first_order_similarity) {
        return 1.0 - volume.g;
    }
    if (use_similarity_relation) {
        return volume.similarity_alpha;
    }
    return 1.0;
}

EvaluatedVolume evaluateVolumeAtLocalPos(VolumeInstance volume, bool use_similarity_relation, bool use_first_order_similarity, vec3 obj_pos) {
    EvaluatedVolume result;

    vec3 vol_uv = posToVolumeUV(volume, obj_pos);

    float alpha = similarityAlpha(volume, use_similarity_relation, use_first_order_similarity);

    result.absorption = getAbsorption(volume, vol_uv);
    result.scattering = alpha * getScattering(volume, vol_uv);
    result.majorant = alpha * volume.max_scattering + volume.max_absorption;
    result.g = volume.g;
    result.similarity_idx = volume.similarity_idx;
    result.similarity_alpha = volume.similarity_alpha;

    return result;
}

float getMaxComponent(vec3 v) {
    return max(v.x, max(v.y, v.z));
}

EvaluatedVolume evaluateHomoVolume(VolumeInstance volume, bool use_similarity_relation, bool use_first_order_similarity) {
     EvaluatedVolume result;

    float alpha = similarityAlpha(volume, use_similarity_relation, use_first_order_similarity);

    result.absorption = volume.avg_absorption.xyz;
    result.scattering = alpha * volume.avg_scattering.xyz;
    result.majorant = alpha * getMaxComponent(volume.avg_absorption.xyz + volume.avg_scattering.xyz);
    result.g = volume.g;
    result.similarity_idx = volume.similarity_idx;
    result.similarity_alpha = volume.similarity_alpha;

    return result;
}

EvaluatedVolume evaluateVertexVolume(PathVertex vertex, bool use_similarity_relation, bool use_first_order_similarity, bool assume_homogenous) {
    VolumeInstance volume = getVolume(vertex.volume_idx);
    if (assume_homogenous) {
        return evaluateHomoVolume(volume, use_similarity_relation, use_first_order_similarity);
    }
    return evaluateVolumeAtLocalPos(volume, use_similarity_relation, use_first_order_similarity, posToVolumeLocal(volume, vertex.P));
}

// Transmittance along a segment known to lie entirely inside one volume
vec3 estimateSegmentTransmittance(vec3 from_P, vec3 to_P, VolumeInstance volume_instance, bool use_similarity_relation, bool use_first_order_similarity, bool assume_homogenous, bool regular_tracking, inout uvec4 rng_state) {
    vec3 dir = to_P - from_P;
    float dist = length(dir);
    if (dist <= 0.0) {
        return vec3(1);
    }
    dir /= dist;

    if (assume_homogenous) {
        EvaluatedVolume volume = evaluateHomoVolume(volume_instance, use_similarity_relation, use_first_order_similarity);
        return exp(-(volume.scattering + volume.absorption) * dist);
    }

    if (regular_tracking) {
        ivec3 res = textureSize(scattering_textures[volume_instance.tex_idx], 0);
        vec3 bb_origin = volume_instance.bounding_box_origin.xyz;
        vec3 bb_extent = volume_instance.bounding_box_extent.xyz;
        vec3 voxel_size = bb_extent / vec3(res);

        vec3 obj_origin = (volume_instance.world_to_object * vec4(from_P, 1.0)).xyz;
        vec3 obj_dir = mat3(volume_instance.world_to_object) * dir;

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
        while (tracked_dist < dist) {
            int axis = 0;
            if (next_boundary.y < next_boundary.x) axis = 1;
            if (next_boundary.z < next_boundary[axis]) axis = 2;

            float seg_end = min(next_boundary[axis], dist);
            float seg_len = max(0.0, seg_end - tracked_dist);

            vec3 voxel_center_obj = bb_origin + (vec3(voxel) + 0.5) * voxel_size;
            EvaluatedVolume vol = evaluateVolumeAtLocalPos(volume_instance, use_similarity_relation, use_first_order_similarity, voxel_center_obj);
            optical_depth += (vol.scattering + vol.absorption) * seg_len;
            tracked_dist = seg_end;

            if (tracked_dist >= dist) break;
            voxel[axis] += step_dir[axis];
            next_boundary[axis] += t_delta[axis];
            if (voxel[axis] < 0 || voxel[axis] >= res[axis]) break;
        }
        return exp(-optical_depth);
    }

    EvaluatedVolume volume = evaluateVolumeAtLocalPos(volume_instance, use_similarity_relation, use_first_order_similarity, posToVolumeLocal(volume_instance, from_P));
    vec3 transmittance = vec3(1);
    float tracked_dist = 0;
    while (true) {
        tracked_dist += sampleDistance(volume.majorant, rng_state);
        if (tracked_dist >= dist) {
            break;
        }
        vec3 curr_pos = from_P + tracked_dist * dir;
        volume = evaluateVolumeAtLocalPos(volume_instance, use_similarity_relation, use_first_order_similarity, posToVolumeLocal(volume_instance, curr_pos));
        transmittance *= (1.0 - (volume.scattering + volume.absorption) / volume.majorant);
    }
    return transmittance;
}

vec3 evaluateSurfaceVertex(PathVertex vertex, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    EvaluatedMaterial material = evaluateVertexMaterial(vertex);
    vec3 light = vec3(0);

    // no direct light sampling or handle light that goes directly to the camera
    bool consider_emission = !options.sample_light || context.specular_bounce || (context.depth == 0 && material.emission_power > 0);

    if (consider_emission && dot(vertex.N, vertex.V) > 0) {
        light += material.emission_color * material.emission_power;
    }

    if (options.sample_light) {
        mat3 TBN = getTBN(vertex.geom_N, vertex.T);
        mat3 transpose_tbn = transpose(TBN);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, options.use_similarity_relation, options.use_first_order_similarity, options.assume_homogenous, rng_state);

        if (SPEC_SAMPLE_BSDF) {
            vec3 f = calcConductorBRDF(wo, wi, material.albedo, material.metallic, material.roughness) * max(dot(vertex.N, L), 0.0);
            if (light_sample.light != vec3(0) && dot(f, f) > 0.0 && dot(transmittance, transmittance) > 0.0) {
                light += transmittance * f * light_sample.light / light_sample.pdf;
            }
        } else {
            vec3 f = computeBsdf(wo, wi, material.albedo, material.metallic, material.roughness, material.eta) * abs(dot(vertex.N, L));
            if (light_sample.light != vec3(0) && dot(f, f) > 0.0 && dot(transmittance, transmittance) > 0.0) {
                light += transmittance * f * light_sample.light / light_sample.pdf;
            }
        }
    }

    return light;
}

vec3 evaluateVolumeVertex(PathVertex vertex, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    if (options.sample_light) {
        EvaluatedVolume volume = evaluateVertexVolume(vertex, options.use_similarity_relation, options.use_first_order_similarity, options.assume_homogenous);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, vertex.volume_idx, options.use_similarity_relation, options.use_first_order_similarity, options.assume_homogenous, rng_state);

        float phase = 0;
        if (options.use_first_order_similarity) {
            phase = INV_4_PI;
        } else if (options.use_similarity_relation) {
            phase = evaluateAlteredPhaseFunctionIdx(vertex.V, L, volume.similarity_idx);
        } else {
            phase = henyeyGreenstein(vertex.V, L, volume.g);
        }
        if (light_sample.light != vec3(0) && phase > 0.0 && dot(transmittance, transmittance) > 0.0) {
            return volume.scattering * transmittance * phase * light_sample.light / light_sample.pdf;
        }
    }

    return vec3(0);
}


#endif
