#define EPSILON 0.005

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 eta;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 1) rayPayloadEXT bool isShadowed;
