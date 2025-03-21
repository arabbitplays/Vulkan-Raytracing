#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#define MAX_RECURSION_DEPTH 4

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/layout.glsl"
#include "options.glsl"

hitAttributeEXT vec3 attribs;

#include "phong_lighting.glsl"


Material getMaterial(uint material_id) {
    uint base_index = 5 * material_id;
    vec4 A = material_buffer.data[base_index];
    vec4 B = material_buffer.data[base_index + 1];
    vec4 C = material_buffer.data[base_index + 2];
    vec4 D = material_buffer.data[base_index + 3];
    vec4 E = material_buffer.data[base_index + 4];

    Material m;
    m.diffuse = A.xyz;
    m.specular = vec3(A.w, B.x, B.y);
    m.ambient = vec3(B.zw, C.x);
    m.reflection = C.yzw;
    m.transmission = D.xyz;
    m.n = D.w;
    m.eta = E.xyz;

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

    vec3 L = sceneData.pointLightPositions[0].xyz - P;
    float distance_to_light = length(L);
    L = normalize(L);
    float incoming_light = sceneData.pointLightPositions[0].w / (distance_to_light * distance_to_light);

    vec3 V = -normalize(gl_WorldRayDirectionEXT);

    int depth = payload.depth;

    vec3 reflection = vec3(0.0);
    if (payload.depth < options.recursion_depth && length(material.reflection) > 0.0) {
        evaluateReflection(P, N, V, material, depth);
        reflection = payload.light;
    }

    vec3 transmission = vec3(0.0);
    if (payload.depth < options.recursion_depth && length(material.transmission) > 0.0) {
        handleTransmissiveMaterial(P, N, V, material, depth);
        transmission = payload.light;
    }

    vec3 phong = evaluatePhong(P, N, L, incoming_light, distance_to_light, V, material);

    payload.light = phong + material.reflection * reflection + material.transmission * transmission + sceneData.ambientColor.xyz * material.diffuse;
}
