#ifndef VOLUME_LAYOUT_GLSL
#define VOLUME_LAYOUT_GLSL

#include "../common/layout.glsl"

struct VolumeInstance {
    float g;
    float majorant; // majorant coefficient used for tracking algorithms
    uint tex_idx;
    float pad;
    vec4 bounding_box_origin;
    vec4 bounding_box_extent;
};

layout(binding = 8, set = 0) buffer VolumeBuffer {
    VolumeInstance volumes[];
} volume_buffer;
layout(binding = 11, set = 0) uniform sampler3D volume_textures[16];

bool isVolumeBoundary(Triangle triangle) {
    return triangle.volume_id != 0;
}

VolumeInstance getVolume(Triangle triangle) {
    return volume_buffer.volumes[triangle.volume_id - 1];
}

VolumeInstance getVolume(int idx) {
    return volume_buffer.volumes[idx];
}

int getVolumeIdx(Triangle triangle) {
    return int(triangle.volume_id) - 1;
}

vec3 posToVolumeUV(VolumeInstance volume, vec3 world_pos, mat4x3 vol_world_to_object) {
    vec3 obj_pos = (vol_world_to_object * vec4(world_pos, 1.0f)).xyz;
    vec3 uv = (obj_pos - volume.bounding_box_origin.xyz) / volume.bounding_box_extent.xyz;
    uv = clamp(uv, vec3(0), vec3(1));
    return uv;
}

vec2 getCoefficients(VolumeInstance volume, vec3 uv) {
    return texture(volume_textures[volume.tex_idx], uv).xy;
}

#endif
