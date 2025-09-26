#ifndef SCENE_DATA_GLSL
#define SCENE_DATA_GLSL

layout(binding = 2, set = 0) uniform SceneData {
    mat4 inv_view;
    mat4 inv_proj;
    mat4 last_view_proj;
    vec4 viewPos;
    vec4 pointLightPositions[4];
    vec3 pointLightColors[4];
    vec4 ambientColor;
    vec4 sunlightDirection; // w for power
    vec4 sunlightColor;
} sceneData;

#endif // SCENE_DATA_GLSL