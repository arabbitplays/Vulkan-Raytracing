#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "payload.glsl"
#include "scene_data.glsl"

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
};

struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    vec3 reflection;
    vec3 transmission;
    float n;
    vec3 eta;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 3, set = 0) readonly buffer VertexBuffer {
    vec4[] data;
} vertex_buffer;
layout(binding = 4, set = 0) readonly buffer IndexBuffer {
    uint indices[];
} index_buffer;
layout(binding = 5, set = 0) readonly buffer GeometryappingBuffer {
    uint indices[];
} geometry_mapping_buffer;
layout(binding = 6, set = 0) readonly buffer InstanceMappingBuffer {
    uint indices[];
} instance_mapping_buffer;
layout(binding = 0, set = 1) readonly buffer MaterialBuffer {
    vec4[] data;
} material_buffer;

layout(location = 0) rayPayloadInEXT Payload hitValue;
layout(location = 1) rayPayloadEXT bool isShadowed;
hitAttributeEXT vec3 attribs;


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

    vec3 V = -normalize(gl_WorldRayDirectionEXT);

    float NdotL = dot(N, L);

    vec3 diffuse = vec3(0);
    vec3 specular = vec3(0);

    float incoming_light = sceneData.pointLightPositions[0].w / (distance_to_light * distance_to_light);

    if (NdotL > 0) {
        float tmin = 0.001;
        float tmax = distance_to_light;
        vec3 origin = P + 0.005 * N;
        vec3 direction = L;
        uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        hitValue.color = vec3(0.0);

        traceRayEXT(topLevelAS, flags, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);

        if (!isShadowed) {
            diffuse = incoming_light * material.diffuse * max(0, NdotL);

            vec3 R = reflect(-L, N);
            float VdotR = dot(V, R);
            specular = incoming_light * material.specular * pow(max(0, VdotR), material.n);
        }
    }

    vec3 ambient = incoming_light * material.ambient;

    hitValue.color = diffuse + specular + ambient;
    //hitValue.color = specular;
    hitValue.intersection = vec4(P, 0.0);
    hitValue.normal = vec4(N, gl_HitTEXT);
}
