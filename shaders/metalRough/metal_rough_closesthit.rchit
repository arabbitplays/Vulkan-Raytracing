#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/layout.glsl"
#include "options.glsl"
#include "../common/random.glsl"
#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../volume/transmittance_estimator.glsl"

layout(binding = 1, set = 1) uniform sampler2D material_textures[64];

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT vec3 attribs;

#include "bsdf_sampler.glsl"
#include "light_sampler.glsl"
#include "../common/path_vertex.glsl"

mat3 getTBN(vec3 geom_N, vec3 T) {
    vec3 bitangent = -normalize(cross(geom_N, T));
    return mat3(T, bitangent, geom_N);
}

PathVertex createPathVertex() {
    PathVertex vertex;

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    vertex.material_idx = triangle.material_idx;
    vertex.volume_idx = getVolumeIdx(triangle);

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x;
    float beta = barycentricCoords.y;
    float gamma = barycentricCoords.z;

    vec3 local_position = alpha * A.position + beta * B.position + gamma * C.position;
    vec3 local_normal = normalize(alpha * A.normal + beta * B.normal + gamma * C.normal);
    vertex.uv = alpha * A.uv + beta * B.uv + gamma * C.uv;

    vertex.P = vec3(gl_ObjectToWorldEXT * vec4(local_position, 1.0)); // transform position to world space

    vec3 geometric_normal = normalize(vec3(local_normal * gl_WorldToObjectEXT)); // transform normal to world space
    vertex.N = geometric_normal;
    vertex.geom_N = geometric_normal;

    vec3 tangent = normalize(alpha * A.tangent + beta * B.tangent + gamma * C.tangent);
    vertex.T = normalize(vec3(tangent * gl_WorldToObjectEXT)); // transform tangent to world space

    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    if (options.normal_mapping) {
        Material material = getMaterial(triangle.material_idx);
        vec3 texNormal = texture(material_textures[material.normal_tex_idx], vertex.uv).xyz;
        texNormal = texNormal * 2.0 - 1.0;

        mat3 TBN = getTBN(vertex.geom_N, vertex.T);
        vertex.N = normalize(TBN * texNormal);
    }

    return vertex;
}

struct EvaluatedVertex {
    vec3 light;
    vec3 beta;
};

struct SampledDirection {
    vec3 dir;
    float dist;
    float pdf;
    vec3 brdf;
};

SampledDirection sampleNextDirection(PathVertex vertex, bool sample_bsdf, inout uvec4 rng_state) {
    SampledDirection sampled_dir;
    sampled_dir.dist = INFINITY;

    mat3 TBN = getTBN(vertex.geom_N, vertex.T);
    mat3 transpose_tbn = transpose(TBN);

    Material material = getMaterial(vertex.material_idx);
    vec3 albedo = texture(material_textures[material.albedo_tex_idx], vertex.uv).xyz + material.albedo;
    vec3 metal_rough_ao = texture(material_textures[material.metal_rough_ao_tex_idx], vertex.uv).xyz;
    float metallic = metal_rough_ao.x + material.metallic;
    float roughness = metal_rough_ao.y + material.roughness;
    float ao = metal_rough_ao.z + material.ao;
    float eta = material.eta;

    if (sample_bsdf) {
        vec3 wo = normalize(transpose_tbn * vertex.V);

        BsdfSample brdf_sample = sampleBsfd(wo, albedo, metallic, roughness, eta, rng_state);

        sampled_dir.dir = TBN * brdf_sample.wi;
        sampled_dir.brdf = brdf_sample.f * abs(dot(sampled_dir.dir, vertex.N));
        sampled_dir.pdf = brdf_sample.pdf;
        payload.specular_bounce = isSpecular(brdf_sample.flags);
        if (isTransmissive(brdf_sample.flags))
            payload.eta_scale *= sqr(brdf_sample.eta);
    } else {
        //payload.next_direction = TBN * sampleCosHemisphere(payload.rng_state);
        sampled_dir.dir = sampleUniformSphere(rng_state);

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * sampled_dir.dir);
        //payload.beta *= calcBRDF(wo, wi, albedo, metallic, roughness) * PI;
        sampled_dir.brdf = computeBsdf(wo, wi, albedo, metallic, roughness, eta) * abs(dot(sampled_dir.dir, vertex.N));
        sampled_dir.pdf = 1.0f / (4.0f * PI);
    }

    return sampled_dir;
}

void main() {
    PathVertex vertex = createPathVertex();
    payload.next_vertex = vertex;

    mat3 TBN = getTBN(vertex.geom_N, vertex.T);
    mat3 transpose_tbn = transpose(TBN);

    if (isVolumeBoundary(vertex.volume_idx)) {
        payload.next_origin = vertex.P;

        VolumeInstance volume = getVolume(vertex.volume_idx);

        if (payload.current_volume_idx >= 0) {
            payload.current_volume_idx = -1;
            payload.next_distance = INFINITY;
        } else {
            payload.current_volume_idx = vertex.volume_idx;
            payload.volume_world_to_object = gl_WorldToObjectEXT;

            payload.next_distance = sampleDistance(volume.majorant, payload.rng_state);
        }
    } else {
        Material material = getMaterial(vertex.material_idx);
        vec3 albedo = texture(material_textures[material.albedo_tex_idx], vertex.uv).xyz + material.albedo;
        vec3 metal_rough_ao = texture(material_textures[material.metal_rough_ao_tex_idx], vertex.uv).xyz;
        float metallic = metal_rough_ao.x + material.metallic;
        float roughness = metal_rough_ao.y + material.roughness;
        float ao = metal_rough_ao.z + material.ao;
        float eta = material.eta;

        // no direct light sampling or handle light that goes directly to the camera
        if (!options.sample_light || payload.specular_bounce || (payload.depth == 0 && material.emission_power > 0)) {
            if (dot(vertex.N, vertex.V) > 0) {
                payload.light += payload.beta * material.emission_color * material.emission_power;
            }
        }

        if (options.sample_light) {
            uint emitter_count = max(1, sceneData.emitter_count);
            LightSample light_sample = sampleEmittingPrimitive(vertex.P, emitter_count, payload.rng_state);
            vec3 L = light_sample.P - vertex.P;
            float distance_to_light = length(L);
            L = normalize(L);

            vec3 wo = normalize(transpose_tbn * vertex.V);
            vec3 wi = normalize(transpose_tbn * L);

            vec3 transmittance = estimateTransmittance(vertex.P, L, distance_to_light, payload.rng_state);
            //payload.light = transmittance;
            //payload.next_direction = vec3(0);
            //return;

            if (options.sample_bsdf) {
                vec3 f = calcConductorBRDF(wo, wi, albedo, metallic, roughness) * max(dot(vertex.N, L), 0.0);
                if (light_sample.light != vec3(0) && length(f) > 0.0 && length(transmittance) > 0) {
                    payload.light += payload.beta * transmittance * f * light_sample.light / light_sample.pdf;
                }
            } else {
                vec3 f = computeBsdf(wo, wi, albedo, metallic, roughness, eta) * abs(dot(vertex.N, L));
                if (light_sample.light != vec3(0) && length(f) > 0.0 && length(transmittance) > 0) {
                    payload.light += payload.beta * transmittance * f * light_sample.light / light_sample.pdf;
                }
            }
        }

        payload.next_origin = vertex.P;

        SampledDirection sampled_dir = sampleNextDirection(vertex, options.sample_bsdf, payload.rng_state);
        payload.next_direction = sampled_dir.dir;
        payload.next_distance = sampled_dir.dist;
        payload.beta *= sampled_dir.brdf / sampled_dir.pdf;

        if (options.russian_roulette) {
            vec3 rr_beta = payload.beta * payload.eta_scale;
            float beta_max_component = max(rr_beta.x, max(rr_beta.y, rr_beta.z));
            if (beta_max_component < 1 && payload.depth > 1) {
                float q = max(0, 1 - beta_max_component);
                float u = stepAndOutputRNGFloat(payload.rng_state);
                if (u < q) {
                    payload.next_direction = vec3(0);
                } else {
                    payload.beta /= 1 - q;
                }
            }
        }
    }
}
