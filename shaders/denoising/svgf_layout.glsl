struct SvgfData {
    vec3 color;
    vec3 position;
    vec3 normal;
    float depth;
    vec2 motion;
};

struct SvgfHistData {
    vec3 color;
    vec3 normal;
    float depth;
};

layout(binding = 0, set = 2) buffer DenoisingBuffer {
    vec4[] data;
} denoising_buffer;

layout(binding = 1, set = 2) buffer DenoisingHistBuffer {
    vec4[] data;
} denoising_hist_buffer;

SvgfData getSvgfData(uint pixel_idx) {
    SvgfData data;
    uint buffer_idx = 3 * pixel_idx;

    vec4 A = denoising_buffer.data[buffer_idx];
    vec4 B = denoising_buffer.data[buffer_idx + 1];
    vec4 C = denoising_buffer.data[buffer_idx + 2];

    data.color = A.xyz;
    data.position = vec3(A.w, B.xy);
    data.normal = vec3(B.zw, C.x);
    data.depth = C.y;
    data.motion = C.zw;

    return data;
}

void setSvgfData(uint pixel_idx, SvgfData data) {
    uint buffer_idx = 3 * pixel_idx;
    vec4 A = vec4(data.color, data.position.x);
    vec4 B = vec4(data.position.yz, data.normal.xy);
    vec4 C = vec4(data.normal.z, data.depth, data.motion);


    denoising_buffer.data[buffer_idx] = A;
    denoising_buffer.data[buffer_idx + 1] = B;
    denoising_buffer.data[buffer_idx + 2] = C;
}

SvgfHistData getSvgfHistData(uint pixel_idx) {
    SvgfHistData data;
    uint buffer_idx = 2 * pixel_idx;

    vec4 A = denoising_hist_buffer.data[buffer_idx];
    vec4 B = denoising_hist_buffer.data[buffer_idx + 1];

    data.color = A.xyz;
    data.normal = vec3(A.w, B.xy);
    data.depth = B.z;

    return data;
}

void setSvgfHistData(uint pixel_idx, SvgfHistData data) {
    uint buffer_idx = 2 * pixel_idx;
    vec4 A = vec4(data.color, data.normal.x);
    vec4 B = vec4(data.normal.yz, data.depth, 0);

    denoising_hist_buffer.data[buffer_idx] = A;
    denoising_hist_buffer.data[buffer_idx + 1] = B;
}