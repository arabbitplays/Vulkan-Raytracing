#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define MAX_RECURSION_DEPTH 4

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/layout.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(binding = 1, set = 1) uniform sampler2D albedo_textures[64];
layout(binding = 2, set = 1) uniform sampler2D metal_rough_ao_textures[64];
layout(binding = 3, set = 1) uniform sampler2D normal_textures[64];


hitAttributeEXT vec3 attribs;

#include "metal_rough_lighting.glsl"



Material getMaterial(uint material_id) {
    uint base_index = 2 * material_id;
    vec4 A = material_buffer.data[base_index];
    vec4 B = material_buffer.data[base_index + 1];

    Material m;
    m.albedo = A.xyz;
    m.metallic = B.x;
    m.roughness = B.y;
    m.ao = B.z;

    return m;
}

void main() {
    uint index = gl_InstanceCustomIndexEXT;

    uint geometry_index = instance_mapping_buffer.indices[2 * index];
    uint material_index = instance_mapping_buffer.indices[2 * index + 1];

    uint vertex_offset = geometry_mapping_buffer.indices[2 * geometry_index];
    uint index_offset = geometry_mapping_buffer.indices[2 * geometry_index + 1];

    uvec3 indices = getIndices(index_offset, gl_PrimitiveID);
    Vertex A = getVertex(vertex_offset, indices.x);
    Vertex B = getVertex(vertex_offset, indices.y);
    Vertex C = getVertex(vertex_offset, indices.z);

    Material material = getMaterial(material_index);

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
    if (options.normal_mapping) {
        vec3 tangent = normalize(alpha * A.tangent + beta * B.tangent + gamma * C.tangent);
        vec3 T = normalize(vec3(tangent * gl_WorldToObjectEXT)); // transform tangent to world space
        vec3 bitangent = -normalize(cross(geometric_normal, T));
        mat3 TBN = mat3(T, bitangent, geometric_normal);
        vec3 texNormal = texture(normal_textures[material_index], uv).xyz;
        texNormal = texNormal * 2.0 - 1.0;
        N = normalize(TBN * texNormal);
    }

    vec3 albedo = texture(albedo_textures[material_index], uv).xyz + material.albedo;
    float metallic = texture(metal_rough_ao_textures[material_index], uv).x + material.metallic;
    float roughness = texture(metal_rough_ao_textures[material_index], uv).y + material.roughness;
    float ao = texture(metal_rough_ao_textures[material_index], uv).z + material.ao;

    // handle point light
    vec3 out_radiance = vec3(0);
    for (int light_index = 0; light_index < 4; light_index++) {
        if (sceneData.pointLightPositions[light_index].w == 0)
            continue;

        vec3 L = sceneData.pointLightPositions[light_index].xyz - P;
        float distance_to_light = length(L);
        float attenuation = 1.0 / (distance_to_light * distance_to_light);
        L = normalize(L);
        out_radiance += calcLightContribution(P, N, geometric_normal, L, distance_to_light, attenuation,
                                              sceneData.pointLightColors[light_index].xyz,
                                              sceneData.pointLightPositions[light_index].w,
                                              albedo, metallic, roughness, ao);
    }

    // handle directional light (sun)
    if (sceneData.sunlightDirection.w != 0) {
        vec3 L = -normalize(sceneData.sunlightDirection.xyz);
        float distance_to_light = length(L);
        float attenuation = 1.0 / (distance_to_light * distance_to_light);
        out_radiance += calcLightContribution(P, N, geometric_normal, L, distance_to_light, attenuation,
                                              sceneData.sunlightColor,
                                              sceneData.sunlightDirection.w,
                                              albedo, metallic, roughness, ao);
    }

    vec3 ambient = ao * vec3(0.01) * albedo;
    vec3 result = ambient + out_radiance;

    payload.light = result;
    // gamma correction
    //payload.light = payload.light / (payload.light + vec3(1.0));
    //payload.light = pow(payload.light, vec3(1.0 / 2.2));
}
