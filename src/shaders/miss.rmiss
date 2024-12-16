#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"

layout(location = 0) rayPayloadInEXT Payload hitValue;

void main() {
    hitValue.color = vec3(0.0, 0.0, 0.2);
    hitValue.normal.w = 10000;
}