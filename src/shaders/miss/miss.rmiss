#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common/payload.glsl"

layout(set = 0, binding = 7) uniform sampler2D textures[6];

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.light = vec3(0.0, 0.0, 0.2);
}