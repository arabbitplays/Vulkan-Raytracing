struct GBufferData {
    vec3 color;
    vec3 position;
    vec3 normal;
    float depth;
    vec2 motion;
    int instance_id;
};

struct GBufferHistData {
    vec3 color;
    vec3 normal;
    float depth;
    int instance_id;
};

layout(binding = 4, set = 1) buffer GBuffer {
    vec4[] data;
} g_buffer;

layout(binding = 5, set = 1) buffer HistGBuffer {
    vec4[] data;
} hist_g_buffer;

GBufferData getGBufferData(uint pixel_idx) {
    GBufferData data;
    uint buffer_idx = 4 * pixel_idx;

    vec4 A = g_buffer.data[buffer_idx];
    vec4 B = g_buffer.data[buffer_idx + 1];
    vec4 C = g_buffer.data[buffer_idx + 2];
    vec4 D = g_buffer.data[buffer_idx + 3];

    data.color = A.xyz;
    data.position = vec3(A.w, B.xy);
    data.normal = vec3(B.zw, C.x);
    data.depth = C.y;
    data.motion = C.zw;
    data.instance_id = int(round(D.x));

    return data;
}

void setGBufferData(uint pixel_idx, GBufferData data) {
    uint buffer_idx = 4 * pixel_idx;
    vec4 A = vec4(data.color, data.position.x);
    vec4 B = vec4(data.position.yz, data.normal.xy);
    vec4 C = vec4(data.normal.z, data.depth, data.motion);
    vec4 D = vec4(data.instance_id, 0, 0, 0);

    g_buffer.data[buffer_idx] = A;
    g_buffer.data[buffer_idx + 1] = B;
    g_buffer.data[buffer_idx + 2] = C;
    g_buffer.data[buffer_idx + 3] = D;
}

GBufferHistData getGBufferHistData(uint pixel_idx) {
    GBufferHistData data;
    uint buffer_idx = 2 * pixel_idx;

    vec4 A = hist_g_buffer.data[buffer_idx];
    vec4 B = hist_g_buffer.data[buffer_idx + 1];

    data.color = A.xyz;
    data.normal = vec3(A.w, B.xy);
    data.depth = B.z;
    data.instance_id = int(round(B.w));

    return data;
}

void setGBufferHistData(uint pixel_idx, GBufferHistData data) {
    uint buffer_idx = 2 * pixel_idx;
    vec4 A = vec4(data.color, data.normal.x);
    vec4 B = vec4(data.normal.yz, data.depth, data.instance_id);

    hist_g_buffer.data[buffer_idx] = A;
    hist_g_buffer.data[buffer_idx + 1] = B;
}

void writePrimaryHitData(uint idx, vec3 P, vec3 N, float depth, int instance_id) {
    GBufferData data = getGBufferData(idx);
    data.position = P;
    data.normal = N;
    data.depth = depth;
    data.instance_id = instance_id;
    setGBufferData(idx, data);
}