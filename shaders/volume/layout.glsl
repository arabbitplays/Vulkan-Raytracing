#ifndef VOLUME_LAYOUT_GLSL
#define VOLUME_LAYOUT_GLSL

#include "../common/layout.glsl"

struct VolumeInstance {
    float g;
    float max_scattering;
    float max_absorption;
    uint tex_idx;
    vec4 bounding_box_origin;
    vec4 bounding_box_extent;
};

layout(binding = 8, set = 0) buffer VolumeBuffer {
    VolumeInstance volumes[];
} volume_buffer;
layout(binding = 11, set = 0) uniform sampler3D scattering_textures[16];
layout(binding = 12, set = 0) uniform sampler3D absorption_textures[16];

int getVolumeIdx(Triangle triangle) {
    return int(triangle.volume_id) - 1;
}

VolumeInstance getVolume(int idx) {
    return volume_buffer.volumes[idx];
}

bool isVolumeBoundary(int volume_idx) {
    return volume_idx >= 0;
}

VolumeInstance getVolume(Triangle triangle) {
    return getVolume(getVolumeIdx(triangle));
}

bool isVolumeBoundary(Triangle triangle) {
    return isVolumeBoundary(getVolumeIdx(triangle));
}

vec3 posToVolumeUV(VolumeInstance volume, vec3 obj_pos) {
    vec3 uv = (obj_pos - volume.bounding_box_origin.xyz) / volume.bounding_box_extent.xyz;
    uv = clamp(uv, vec3(0), vec3(1));
    return uv;
}

vec3 getScattering(VolumeInstance volume, vec3 uv) {
    return texture(scattering_textures[volume.tex_idx], uv).xyz;
}

vec3 getAbsorption(VolumeInstance volume, vec3 uv) {
    return texture(absorption_textures[volume.tex_idx], uv).xyz;
}

#endif
