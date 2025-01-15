#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../common/payload.glsl"
#include "../common/scene_data.glsl"

layout(set = 0, binding = 7) uniform sampler2D textures[6];

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.light = sceneData.sunlightColor * sceneData.sunlightDirection.w;
}