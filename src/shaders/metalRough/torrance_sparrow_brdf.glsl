#include "../common/constants.glsl"

layout(location = 1) rayPayloadEXT bool isShadowed;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 eta;
    vec3 emission_color;
    float emission_power;
};

Material getMaterial(uint material_id) {
    uint base_index = 3 * material_id;
    vec4 A = material_buffer.data[base_index];
    vec4 B = material_buffer.data[base_index + 1];
    vec4 C = material_buffer.data[base_index + 2];

    Material m;
    m.albedo = A.xyz;
    m.metallic = B.x;
    m.roughness = B.y;
    m.ao = B.z;
    m.emission_color = C.xyz;
    m.emission_power = C.w;

    return m;
}

// Throwbridge Reitz Distribution
float D(vec3 wm, vec3 n, float alpha) {
    float cos_theta = dot(n, wm);
    float cos2_theta = cos_theta * cos_theta;
    float sin2_theta = 1 - cos2_theta;
    float tan2_theta = sin2_theta / cos2_theta;
    if (isinf(tan2_theta)) return 0;
    float cos4_theta = cos2_theta * cos2_theta;
    float alpha2 = alpha * alpha;
    float e = 1 + tan2_theta / alpha2;
    float denom = PI * alpha2 * cos4_theta * e * e;
    return 1 / denom;
}

float lambda(vec3 w, vec3 n, float alpha) {
    float cos_theta = dot(n, w);
    float cos2_theta = cos_theta * cos_theta;
    float sin2_theta = 1 - cos2_theta;
    float tan2_theta = sin2_theta / cos2_theta;
    if (isinf(tan2_theta)) return 0;
    float alpha2 = alpha * alpha;
    float nom = sqrt(1 + alpha2 * tan2_theta) - 1;
    return nom / 2;
}

float G(vec3 n, vec3 wo, vec3 wi, float alpha) {
    return 1 / (1 + lambda(wo, n, alpha) + lambda(wi, n, alpha));
}

float G1(vec3 n, vec3 w, float alpha) {
    return 1 / (1 + lambda(w, n, alpha));
}

float D(vec3 w, vec3 wm, vec3 n, float alpha) {
    return G1(n, w, alpha) * abs(dot(w, n)) * D(wm, n, alpha) * abs(dot(w, wm));
}

vec3 sampleWm(vec3 wo, inout uvec4 rngState) {
    return vec3(0.0);
}

float normalDistributionPDF(vec3 w, vec3 wm, vec3 n, float alpha) {
    return D(w, wm, n, alpha);
}


vec3 fresnelSchlick(vec3 h, vec3 v, vec3 albedo, float metallic) {
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);
    float HdotV = max(dot(h, v), 0.0);
    return f0 + (1.0 - f0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

vec3 calcBRDF(vec3 n, vec3 wo, vec3 wi, vec3 albedo, float metallic, float roughness) {
    if (dot(n, wo) * dot(n, wi) < 0) {
        return vec3(0);
    }

    vec3 wm = normalize(wi + wo);

    // torance sparrow brdf
    float normalDistributionFunction = D(wm, n, roughness);
    float geometryFunction = G(n, wo, wi, roughness);
    vec3 fresnel = fresnelSchlick(wm, wo, albedo, metallic);

    float NdotV = max(dot(wo, n), 0.0);
    float NdotL = max(dot(wi, n), 0.0);
    float denom = 4 * NdotV * NdotL + 0.0001;
    vec3 nom = normalDistributionFunction * geometryFunction * fresnel;

    vec3 specular = nom / denom;

    vec3 specularFraction = fresnel;
    vec3 diffuseFraction = vec3(1.0) - specularFraction;
    diffuseFraction *= 1.0 - metallic;

    return diffuseFraction * albedo / PI + specular;
}

struct BrdfSample {
    vec3 f;
    vec3 wi;
    float pdf;
};

BrdfSample sampleBrdf(vec3 n, vec3 wo, vec3 albedo, float metallic, float roughness, inout uvec4 rngState) {
    BrdfSample result = BrdfSample(vec3(0.0), vec3(0.0), 0);

    vec3 wm = sampleWm(wo, rngState);
    vec3 wi = reflect(-wo, wm);

    if (dot(wo, wi) < 0) return result;

    result.pdf = normalDistributionPDF(wo, wm, n, roughness) / (4 * abs(dot(wo, wm)));
    result.f = calcBRDF(n, wo, wi, albedo, metallic, roughness);
    result.wi = wi;

    return result;
}