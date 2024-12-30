#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common/payload.glsl"

layout(location = 0) rayPayloadInEXT bool isShadowed;

void main() {
    isShadowed = false;
}