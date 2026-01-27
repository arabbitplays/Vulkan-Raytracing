layout(binding = 2, set = 0) uniform SceneData {
    mat4 inv_view;
    mat4 inv_proj;
    vec4 viewPos;
    vec4 pointLightPositions[4];
    vec3 pointLightColors[4];
    vec4 ambientColor;
    vec4 sunlightDirection; // w for power
    vec4 sunlightColor;
    uint emitter_count;

} sceneData;
