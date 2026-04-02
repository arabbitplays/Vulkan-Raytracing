#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "./shadow_payload.glsl"

layout(location = 0) rayPayloadInEXT ShadowPayload payload;

void main() {
    payload.transmittance = vec3(1);
}