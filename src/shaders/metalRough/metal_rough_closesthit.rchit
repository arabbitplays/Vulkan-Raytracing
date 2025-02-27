#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/layout.glsl"
#include "../common/options.glsl"
#include "../common/random.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 1, set = 1) uniform sampler2D albedo_textures[64];
layout(binding = 2, set = 1) uniform sampler2D metal_rough_ao_textures[64];
layout(binding = 3, set = 1) uniform sampler2D normal_textures[64];


hitAttributeEXT vec3 attribs;

#include "torrance_sparrow_brdf.glsl"
#include "light_sampler.glsl"

void main() {
    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    Material material = getMaterial(triangle.material_idx);

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x;
    float beta = barycentricCoords.y;
    float gamma = barycentricCoords.z;

    vec3 position = alpha * A.position + beta * B.position + gamma * C.position;
    vec3 normal = normalize(alpha * A.normal + beta * B.normal + gamma * C.normal);
    vec3 color = alpha * A.color + beta * B.color + gamma * C.color;
    vec2 uv = alpha * A.uv + beta * B.uv + gamma * C.uv;

    vec3 P = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0)); // transform position to world space
    vec3 geometric_normal = normalize(vec3(normal * gl_WorldToObjectEXT)); // transform normal to world space

    vec3 N = geometric_normal;
    vec3 tangent = normalize(alpha * A.tangent + beta * B.tangent + gamma * C.tangent);
    vec3 T = normalize(vec3(tangent * gl_WorldToObjectEXT)); // transform tangent to world space
    vec3 bitangent = -normalize(cross(geometric_normal, T));
    mat3 TBN = mat3(T, bitangent, geometric_normal);
    if (options.normal_mapping) {
        vec3 texNormal = texture(normal_textures[triangle.material_idx], uv).xyz;
        texNormal = texNormal * 2.0 - 1.0;
        N = normalize(TBN * texNormal);
    }

    vec3 V = -normalize(gl_WorldRayDirectionEXT);

    vec3 albedo = texture(albedo_textures[triangle.material_idx], uv).xyz + material.albedo;
    float metallic = texture(metal_rough_ao_textures[triangle.material_idx], uv).x + material.metallic;
    float roughness = texture(metal_rough_ao_textures[triangle.material_idx], uv).y + material.roughness;
    float ao = texture(metal_rough_ao_textures[triangle.material_idx], uv).z + material.ao;

    // no direct light sampling or handle light that goes directly to the camera
    if (!options.sample_light || (payload.depth == 0 && material.emission_power > 0)) {
        if (dot(N, V) > 0) {
            payload.light += payload.beta * material.emission_color * material.emission_power;
        }
    }

    if (options.sample_light) {
        LightSample light_sample = sampleEmittingPrimitive(P);
        vec3 L = light_sample.P - P;
        float distance_to_light = length(L);
        L = normalize(L);

        vec3 f = calcBRDF(N, V, L, albedo, metallic, roughness) * max(dot(N, L), 0.0);
        if (light_sample.light != vec3(0) && length(f) > 0.0 && unoccluded(P, L, distance_to_light)) {
            payload.light += payload.beta * f * light_sample.light / light_sample.pdf;
        }
    }

    payload.next_origin = P;
    payload.next_direction = TBN * sampleCosHemisphere(payload.rng_state);
    /*payload.next_direction = sampleUniformSphere(payload.rng_state);
    if (dot(payload.next_direction, N) < 0) {
        payload.next_direction = -payload.next_direction;
    }*/

    /*payload.next_direction = vec3(0);
    for (int i = 0; i < 1000; i++) {
        vec3 sampled_dir = TBN * sampleCosHemisphere(payload.rng_state);

        if (dot(sampled_dir, N) < 0) {
            sampled_dir = -sampled_dir;
        }
        payload.next_direction += sampled_dir;
    }
    payload.next_direction = normalize(payload.next_direction);*/

    // f * cos / PDF
    payload.beta *= calcBRDF(N, V, payload.next_direction, albedo, metallic, roughness) * PI;
    //payload.beta *= calcBRDF(N, V, payload.next_direction, albedo, metallic, roughness) * max(0.0, dot(payload.next_direction, N)) * 2 * PI;
    //payload.beta *= calcBRDF(N, V, payload.next_direction, albedo, metallic, roughness) * max(0.0, dot(payload.next_direction, N)) * 4 * PI;
}
