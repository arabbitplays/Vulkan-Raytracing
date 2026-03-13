#ifndef VOLUME_LAYOUT
#define VOLUME_LAYOUT

#include "../common/layout.glsl"

layout(binding = 8, set = 0) buffer VolumeBuffer {
    VolumeInstance volumes[];
} volume_buffer;

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

#endif