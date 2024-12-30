#version 450

#extension GL_GOOGLE_include_directive : enable

#include "input_structures.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;

layout( push_constant ) uniform constants
{
    mat4 model;
} objectData;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec2 outUV;

void main() {
    gl_Position = sceneData.proj * sceneData.view * objectData.model * vec4(inPosition, 1.0);
    outColor = inColor * materialData.colorFactors.xyz;
    outUV = inUV;
    mat3 normalMatrix = mat3(transpose(inverse(objectData.model)));
    outNormal = normalMatrix * inNormal;
    outPos = (objectData.model  * vec4(inPosition, 1.0)).xyz;
}
