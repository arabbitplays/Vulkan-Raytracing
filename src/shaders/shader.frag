#version 450

#extension GL_GOOGLE_include_directive : enable

#include "input_structures.glsl"
#include "lighting.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColor;


void main() {
    vec3 n = normalize(inNormal);
    vec3 v = normalize(sceneData.viewPos.xyz - inPos);

    vec3 albedo = inColor * texture(colorTex, inUV).xyz;
    // TODO include texture
    float metallic = materialData.metalRoughFactors.x;
    float roughness = materialData.metalRoughFactors.y;

    vec3 outRadiance = vec3(0.0f);
    for (int i = 0; i < POINT_LIGHT_COUNT; i++) {
        vec3 l = normalize(sceneData.pointLightPositions[i].xyz - inPos);
        vec3 h = normalize(l + v);

        float distance = length(sceneData.pointLightPositions[i].xyz - inPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 inRadiance = sceneData.pointLightColors[i] * attenuation * sceneData.pointLightPositions[i].w;

        vec3 brdf = calcBRDF(l, v, n, h, albedo, metallic, roughness);
        outRadiance += brdf * inRadiance * max(dot(n, l), 0.0);
    }

    vec3 l = normalize(sceneData.sunlightDirection.xyz);
    vec3 h = normalize(l + v);

    vec3 inRadiance = sceneData.sunlightColor * sceneData.sunlightDirection.w;

    vec3 brdf = calcBRDF(l, v, n, h, albedo, metallic, roughness);
    outRadiance += brdf * inRadiance * max(dot(n, l), 0.0);

    vec3 ambient = vec3(0.01) * albedo;
    vec3 color = ambient + outRadiance;

    // gamma correction
    //color = color / (color + vec3(1.0));
    //color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}