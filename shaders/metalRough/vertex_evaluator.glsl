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

EvaluatedVolume evaluateVolumeAtLocalPos(VolumeInstance volume, bool use_similarity_relation, vec3 obj_pos) {
    EvaluatedVolume result;

    vec3 vol_uv = posToVolumeUV(volume, obj_pos);

    float alpha = 1.0;
    if (use_similarity_relation) {
        alpha = getSimilarityRelationsAlpha(volume.g);
    }

    result.absorption = getAbsorption(volume, vol_uv);
    result.scattering = alpha * getScattering(volume, vol_uv);
    result.majorant = alpha * volume.max_scattering + volume.max_absorption;
    result.g = volume.g;

    return result;
}

float getMaxComponent(vec3 v) {
    return max(v.x, max(v.y, v.z));
}

EvaluatedVolume evaluateHomoVolume(VolumeInstance volume, bool use_similarity_relation) {
     EvaluatedVolume result;

    float alpha = 1.0;
    if (use_similarity_relation) {
        alpha = getSimilarityRelationsAlpha(volume.g);
    }

    result.absorption = volume.avg_absorption.xyz;
    result.scattering = alpha * volume.avg_scattering.xyz;
    result.majorant = alpha * getMaxComponent(volume.avg_absorption.xyz + volume.avg_scattering.xyz);
    result.g = volume.g;

    return result;
}

EvaluatedVolume evaluateVertexVolume(PathVertex vertex, bool use_similarity_relation) {
    VolumeInstance volume = getVolume(vertex.volume_idx);
    return evaluateVolumeAtLocalPos(volume, use_similarity_relation, vertex.local_volume_pos);
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

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, options.use_similarity_relation, rng_state);

        if (options.sample_bsdf) {
            vec3 f = calcConductorBRDF(wo, wi, material.albedo, material.metallic, material.roughness) * max(dot(vertex.N, L), 0.0);
            if (light_sample.light != vec3(0) && length(f) > 0.0 && length(transmittance) > 0) {
                light += transmittance * f * light_sample.light / light_sample.pdf;
            }
        } else {
            vec3 f = computeBsdf(wo, wi, material.albedo, material.metallic, material.roughness, material.eta) * abs(dot(vertex.N, L));
            if (light_sample.light != vec3(0) && length(f) > 0.0 && length(transmittance) > 0) {
                light += transmittance * f * light_sample.light / light_sample.pdf;
            }
        }
    }

    return light;
}

vec3 evaluateVolumeVertex(PathVertex vertex, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    if (options.sample_light) {
        EvaluatedVolume volume = evaluateVertexVolume(vertex, options.use_similarity_relation);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, vertex.volume_idx, options.use_similarity_relation, rng_state);

        float phase = 0;
        if (options.use_similarity_relation) {
            phase = evaluateAlteredPhaseFunction(vertex.V, L, volume.g);
        } else {
            phase = henyeyGreenstein(vertex.V, L, volume.g);
        }
        if (light_sample.light != vec3(0) && phase > 0.0 && length(transmittance) > 0) {
            return volume.scattering * transmittance * phase * light_sample.light / light_sample.pdf;
        }
    }

    return vec3(0);
}


#endif
