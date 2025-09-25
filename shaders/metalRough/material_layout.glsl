#ifndef MATERIAL_LAYOUT_GLSL
#define MATERIAL_LAYOUT_GLSL

struct Material {
    vec3 albedo;
    float padding;
    float metallic;
    float roughness;
    float ao;
    float eta;
    vec3 emission_color;
    float emission_power;
    int albedo_tex_idx;
    int metal_rough_ao_tex_idx;
    int normal_tex_idx;
};

layout(binding = 0, set = 1) readonly buffer MaterialBuffer {
    Material[] data;
} material_buffer;

layout(binding = 1, set = 1) uniform sampler2D albedo_textures[16];
layout(binding = 2, set = 1) uniform sampler2D metal_rough_ao_textures[16];
layout(binding = 3, set = 1) uniform sampler2D normal_textures[16];

layout(binding = 4, set = 1) buffer GBuffer {
    vec4[] data;
} g_buffer;

layout(binding = 5, set = 1) buffer HistGBuffer {
    vec4[] data;
} hist_g_buffer;

#endif // MATERIAL_LAYOUT_GLSL