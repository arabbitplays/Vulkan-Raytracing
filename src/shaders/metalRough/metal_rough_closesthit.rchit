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


hitAttributeEXT vec3 attribs;

#include "metal_rough_lighting.glsl"

Vertex getVertex(uint vertexOffset, uint index)
{
    uint base_index = 3 * (vertexOffset + index);
    vec4 A = vertex_buffer.data[base_index];
    vec4 B = vertex_buffer.data[base_index + 1];
    vec4 C = vertex_buffer.data[base_index + 2];

    Vertex v;
    v.position = A.xyz;
    v.normal = vec3(A.w, B.x, B.y);
    v.color = vec3(B.zw, C.x);
    v.uv = C.yz;

    return v;
}

uvec3 getIndices(uint index_offset, uint primitive_id) {
    uint base_index = index_offset + 3 * primitive_id;
    uint index0 = index_buffer.indices[base_index];
    uint index1 = index_buffer.indices[base_index + 1];
    uint index2 = index_buffer.indices[base_index + 2];
    return uvec3(index0, index1, index2);
}

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
    vec3 N = normalize(vec3(normal * gl_WorldToObjectEXT)); // transform normal to world space

    vec3 albedo = texture(albedo_textures[material_index], uv).xyz + material.albedo;
    float metallic = texture(metal_rough_ao_textures[material_index], uv).x + material.metallic;
    float roughness = texture(metal_rough_ao_textures[material_index], uv).y + material.roughness;
    float ao = texture(metal_rough_ao_textures[material_index], uv).z + material.ao;

    vec3 out_radiance = vec3(0);
    for (int light_index = 0; light_index < 4; light_index++) {
        if (sceneData.pointLightPositions[light_index].w == 0)
            continue;

        vec3 L = sceneData.pointLightPositions[light_index].xyz - P;
        float distance_to_light = length(L);
        float attenuation = 1.0 / (distance_to_light * distance_to_light);
        L = normalize(L);

        vec3 V = -normalize(gl_WorldRayDirectionEXT);
        vec3 H = normalize(L + V);

        int depth = payload.depth;

        float NdotL = dot(N, L);

        vec3 in_radiance = vec3(0);;

        if (NdotL > 0) {
            float tmin = 0.001;
            float tmax = distance_to_light;
            vec3 origin = P + EPSILON * N;
            vec3 direction = L;
            uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
            isShadowed = true;

            traceRayEXT(topLevelAS, flags, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);

            if (!isShadowed || !options.shadows) {
                in_radiance = sceneData.pointLightColors[light_index].xyz * sceneData.pointLightPositions[light_index].w * attenuation;
            }
        }

        vec3 brdf = calcBRDF(N, V, L, H, albedo, metallic, roughness);
        out_radiance += brdf * in_radiance * max(dot(N, L), 0.0);
    }

    vec3 ambient = vec3(0.01) * albedo;
    vec3 result = ambient + out_radiance;

    payload.light = result;

    // gamma correction
    //payload.light = payload.light / (payload.light + vec3(1.0));
    //payload.light = pow(payload.light, vec3(1.0 / 2.2));
}
