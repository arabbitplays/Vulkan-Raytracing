#ifndef VERTEX_EVALUATOR_GLSL
#define VERTEX_EVALUATOR_GLSL

#include "../common/path_vertex.glsl"
#include "../common/scene_data.glsl"

#include "options.glsl"
#include "material.glsl"
#include "light_sampler.glsl"
#include "bsdf_sampler.glsl"

#include "../volume/layout.glsl"
#include "../volume/phase_function.glsl"
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

EvaluatedVolume evaluateVolumeAtLocalPos(VolumeInstance volume, vec3 obj_pos) {
    EvaluatedVolume result;

    vec3 vol_uv = posToVolumeUV(volume, obj_pos);

    result.absorption = getAbsorption(volume, vol_uv);
    result.scattering = getScattering(volume, vol_uv);
    result.majorant = volume.majorant;
    result.g = volume.g;

    return result;
}

EvaluatedVolume evaluateVertexVolume(PathVertex vertex) {
    VolumeInstance volume = getVolume(vertex.volume_idx);
    return evaluateVolumeAtLocalPos(volume, vertex.local_volume_pos);
}

vec3 evaluateSurfaceVertex(PathVertex vertex, bool sample_bsdf, bool sample_light, bool consider_emission, inout uvec4 rng_state) {
    EvaluatedMaterial material = evaluateVertexMaterial(vertex);
    vec3 light = vec3(0);

    // no direct light sampling or handle light that goes directly to the camera
    if (consider_emission && dot(vertex.N, vertex.V) > 0) {
        light += material.emission_color * material.emission_power;
    }

    if (sample_light) {
        mat3 TBN = getTBN(vertex.geom_N, vertex.T);
        mat3 transpose_tbn = transpose(TBN);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, rng_state);

        if (sample_bsdf) {
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

vec3 evaluateVolumeVertex(PathVertex vertex, inout uvec4 rng_state) {
    if (options.sample_light) {
        EvaluatedVolume volume = evaluateVertexVolume(vertex);

        uint emitter_count = max(1, sceneData.emitter_count);
        LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, rng_state);
        vec3 L = light_sample.P - vertex.P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, vertex.volume_idx, rng_state);
        float phase = henyeyGreenstein(vertex.V, L, volume.g);
        if (light_sample.light != vec3(0) && phase > 0.0 && length(transmittance) > 0) {
            return volume.scattering * transmittance * phase * light_sample.light / light_sample.pdf;
        }
    }

    return vec3(0);
}

#endif