#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable


layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 1) uniform sampler2D material_textures[64];

#include "./shadow_payload.glsl"
#include "../volume/layout.glsl"
#include "../common/constants.glsl"

layout(location = 0) rayPayloadInEXT ShadowPayload payload;

hitAttributeEXT vec3 attribs;

void main() {
    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    if (isVolumeBoundary(triangle)) {
        payload.transmittance = vec3(1);
        return;
    } else {
        return;
    }
    float distance_to_light = payload.dist_left - gl_HitTEXT;
    float tmin = EPSILON;
    float tmax = distance_to_light - EPSILON;
    vec3 direction = payload.direction;
    vec3 origin = payload.origin + gl_HitTEXT * direction;
    uint flags = gl_RayFlagsOpaqueEXT;

    payload.dist_left = distance_to_light - EPSILON;

//    traceRayEXT(topLevelAS, flags, 0xff, 1, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);
}
