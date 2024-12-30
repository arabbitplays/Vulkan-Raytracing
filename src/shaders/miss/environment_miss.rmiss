#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common/payload.glsl"

layout(set = 0, binding = 7) uniform sampler2D textures[6];

layout(location = 0) rayPayloadInEXT Payload payload;

vec3 evaluateCubeMap() {
    vec3 v = normalize(gl_WorldRayDirectionEXT);
    vec3 vAbs = abs(v);
    float ma;
    vec2 uv;
    int faceIndex;
    if (vAbs.z >= vAbs.x && vAbs.z >= vAbs.y) {
        faceIndex = v.z < 0.0 ? 5 : 4;  // Negative Z: 5, Positive Z: 4
        ma = 0.5 / vAbs.z;
        uv = vec2(v.z < 0.0 ? -v.x : v.x, -v.y);  // Negative Z: invert x
    }
    else if(vAbs.y >= vAbs.x)
    {
        faceIndex = v.y < 0.0 ? 3 : 2;
        ma = 0.5 / vAbs.y;
        uv = vec2(v.x, v.y < 0.0 ? -v.z : v.z);
    }
    else
    {
        faceIndex = v.x < 0.0 ? 1 : 0;
        ma = 0.5 / vAbs.x;
        uv = vec2(v.x < 0.0 ? v.z : -v.z, -v.y);
    }
    vec2 st = uv * ma + 0.5;
    return texture(textures[faceIndex], st).xyz;
}

void main() {
    payload.direct_light = evaluateCubeMap();
}