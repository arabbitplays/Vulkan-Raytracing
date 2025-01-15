layout(location = 0) rayPayloadInEXT Payload payload;

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 color;
    vec2 uv;
};

layout(binding = 3, set = 0) readonly buffer VertexBuffer {
    vec4[] data;
} vertex_buffer;
layout(binding = 4, set = 0) readonly buffer IndexBuffer {
    uint indices[];
} index_buffer;
layout(binding = 5, set = 0) readonly buffer GeometryMappingBuffer {
    uint indices[];
} geometry_mapping_buffer;
layout(binding = 6, set = 0) readonly buffer InstanceMappingBuffer {
    uint indices[];
} instance_mapping_buffer;
layout(binding = 0, set = 1) readonly buffer MaterialBuffer {
    vec4[] data;
} material_buffer;

Vertex getVertex(uint vertexOffset, uint index)
{
    uint base_index = 4 * (vertexOffset + index);
    vec4 A = vertex_buffer.data[base_index];
    vec4 B = vertex_buffer.data[base_index + 1];
    vec4 C = vertex_buffer.data[base_index + 2];
    vec4 D = vertex_buffer.data[base_index + 3];

    Vertex v;
    v.position = A.xyz;
    v.normal = vec3(A.w, B.x, B.y);
    v.tangent = vec3(B.zw, C.x);
    v.color = vec3(C.yz, D.x);
    v.uv = D.yz;

    return v;
}

uvec3 getIndices(uint index_offset, uint primitive_id) {
    uint base_index = index_offset + 3 * primitive_id;
    uint index0 = index_buffer.indices[base_index];
    uint index1 = index_buffer.indices[base_index + 1];
    uint index2 = index_buffer.indices[base_index + 2];
    return uvec3(index0, index1, index2);
}