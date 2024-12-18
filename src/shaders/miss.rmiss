#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.direct_light = vec3(0.0, 0.0, 0.2);
}