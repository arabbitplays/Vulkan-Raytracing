#ifndef VOLUME_LAYOUT
#define VOLUME_LAYOUT

#include "../common/layout.glsl"

struct VolumeInstance {
    float g;
    uint tex_idx;
    vec2 pad;
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

vec2 getCoefficients(VolumeInstance volume, vec3 world_pos) {
    return texture(volume_textures[volume.tex_idx], uvec3(0)).xy;
}

#endif