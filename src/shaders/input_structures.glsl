#define POINT_LIGHT_COUNT 4

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 viewPos;
    vec4 pointLightPositions[POINT_LIGHT_COUNT];
    vec3 pointLightColors[POINT_LIGHT_COUNT];
    vec4 ambientColor;
    vec4 sunlightDirection; // w for power
    vec3 sunlightColor;
} sceneData;

layout(set = 1, binding = 0) uniform MaterialData {
    vec4 colorFactors;
    vec4 metalRoughFactors;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;