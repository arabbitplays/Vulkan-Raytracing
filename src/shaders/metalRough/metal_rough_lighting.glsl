#define EPSILON 0.005
#define PI 3.14159265

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 eta;
    vec3 emission_color;
    float emission_power;
};

vec3 getLambertianDiffuse(vec3 albedo) {
    return albedo / PI;
}

float distributionGGX(vec3 n, vec3 h, float alpha) {
    float a2 = alpha * alpha;
    float NdotH = max(dot(n, h), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float k) {
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float geometrySchlick(vec3 n, vec3 v, vec3 l, float roughness) {
    float k = (roughness + 1) * (roughness + 1) / 8;
    float NdotV = max(dot(n, v), 0.0);
    float NdotL = max(dot(n, l), 0.0);
    return geometrySchlickGGX(NdotV, k) * geometrySchlickGGX(NdotL, k);
}

vec3 fresnelSchlick(vec3 h, vec3 v, vec3 albedo, float metallic) {
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);
    float HdotV = max(dot(h, v), 0.0);
    return f0 + (1.0 - f0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

vec3 calcBRDF(vec3 n, vec3 v, vec3 l, vec3 h, vec3 albedo, float metallic, float roughness) {
    // cook-torance brdf
    float normalDistributionFunction = distributionGGX(n, h, roughness);
    float geometryFunction = geometrySchlick(n, v, l, roughness);
    vec3 fresnel = fresnelSchlick(h, v, albedo, metallic);

    float NdotV = max(dot(v, n), 0.0);
    float NdotL = max(dot(l, n), 0.0);
    float denom = 4 * NdotV * NdotL + 0.0001;
    vec3 nom = normalDistributionFunction * geometryFunction * fresnel;

    vec3 specular = nom / denom;

    vec3 specularFraction = fresnel;
    vec3 diffuseFraction = vec3(1.0) - specularFraction;
    diffuseFraction *= 1.0 - metallic;

    return diffuseFraction * albedo / PI + specular;
}

vec3 calcLightContribution(vec3 P, vec3 N, vec3 geom_N, vec3 L, float dist_to_light, float attenuation, vec3 light_color, float light_power,
        vec3 albedo, float metallic, float roughness, float ao) {
    vec3 V = -normalize(gl_WorldRayDirectionEXT);
    vec3 H = normalize(L + V);

    vec3 in_radiance = vec3(0);;

    float NdotL = dot(N, L);
    if (NdotL > 0) {
        float tmin = 0.001;
        float tmax = dist_to_light;
        vec3 direction = L;
        vec3 origin = P + EPSILON * geom_N;
        uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;

        traceRayEXT(topLevelAS, flags, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);

        if (!isShadowed || !options.shadows) {
            in_radiance = light_color * light_power * attenuation;
        }
    }

    vec3 brdf = calcBRDF(N, V, L, H, albedo, metallic, roughness);
    return brdf * in_radiance * max(dot(N, L), 0.0);
}