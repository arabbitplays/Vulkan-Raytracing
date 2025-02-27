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
float D(vec3 wm, float alpha) {
    float tan2_theta = tan2Theta(wm);
    if (isinf(tan2_theta)) return 0;
    float cos4_theta = sqr(cos2Theta(wm));
    float alpha2 = alpha * alpha;
    float e = tan2_theta / alpha2;
    return 1 / (PI * alpha2 * cos4_theta * sqr(e + 1));
}

float lambda(vec3 w, float alpha) {
    float tan2_theta = tan2Theta(w);
    if (isinf(tan2_theta)) return 0;
    float alpha2 = alpha * alpha;
    float nom = sqrt(1 + alpha2 * tan2_theta) - 1;
    return nom / 2;
}

float G( vec3 wo, vec3 wi, float alpha) {
    return 1 / (1 + lambda(wo, alpha) + lambda(wi, alpha));
}

float G1(vec3 w, float alpha) {
    return 1 / (1 + lambda(w, alpha));
}

float D(vec3 w, vec3 wm, float alpha) {
    return G1(w, alpha) * absCosTheta(w) * D(wm, alpha) * abs(dot(w, wm));
}

vec3 sampleWm(vec3 w, float alpha, inout uvec4 rngState) {
    vec3 wh = normalize(vec3(alpha * w.x, alpha * w.y, w.z));
    if (wh.z < 0) wh = -wh;

    vec3 t1 = (wh.z < 0.99999) ? normalize(cross(vec3(0, 0, 1), wh)) : vec3(1, 0, 0);
    vec3 t2 = cross(wh, t1);

    vec2 p = sampleUniformDiskPolar(rngState);

    float h = sqrt(1 - sqr(p.x));
    p.y = mix(h, p.y, (1 + wh.z) / 2);

    float pz = sqrt(max(0, 1 - lengthSquared(p)));
    vec3 nh = p.x * t1 + p.y * t2 + pz * wh;

    return normalize(vec3(alpha * nh.x, alpha * nh.y, max(1.0E-6, nh.z)));
}

float normalDistributionPDF(vec3 w, vec3 wm, float alpha) {
    return D(w, wm, alpha);
}

vec3 fresnelSchlick(vec3 h, vec3 v, vec3 albedo, float metallic) {
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);
    float HdotV = max(dot(h, v), 0.0);
    return f0 + (1.0 - f0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

bool effectivelySmooth(float alpha) {
    return alpha < 1E-3;
}

vec3 calcBRDF(vec3 wo, vec3 wi, vec3 albedo, float metallic, float roughness) {
    if (!sameHemisphere(wo, wi) || effectivelySmooth(roughness)) return vec3(0);

    float cos_theta_o = cosTheta(wo);
    float cos_theta_i = cosTheta(wi);
    if (cos_theta_i == 0 || cos_theta_o == 0) return vec3(0);

    vec3 wm = wi + wo;
    if (lengthSquared(wm) == 0) return vec3(0);
    wm = normalize(wm);

    // torrance sparrow brdf
    vec3 fresnel = fresnelSchlick(wm, wo, albedo, metallic);
    vec3 specular = D(wm, roughness) * fresnel * G(wo, wi, roughness) / (4 * cos_theta_o * cos_theta_i);

    vec3 specularFraction = fresnel;
    vec3 diffuseFraction = vec3(1.0) - specularFraction;
    diffuseFraction *= 1.0 - metallic;

    return diffuseFraction * albedo / PI + specular;
}

struct BrdfSample {
    vec3 f;
    vec3 wi;
    float pdf;
    bool isSpecular;
};

BrdfSample sampleBrdf(vec3 wo, vec3 albedo, float metallic, float roughness, inout uvec4 rngState) {
    BrdfSample result = BrdfSample(vec3(0.0), vec3(0.0), 1, false);

    if (effectivelySmooth(roughness)) {
        vec3 wi = vec3(-wo.x, -wo.y, wo.z);
        result.f = fresnelSchlick(vec3(0, 0, 1), wo, albedo, metallic) / absCosTheta(wi);
        result.pdf = 1;
        result.wi = wi;
        result.isSpecular = true;
        return result;
    }

    vec3 wm = sampleWm(wo, roughness, rngState);
    vec3 wi = reflect(-wo, wm);
    if (!sameHemisphere(wo, wm)) return result;

    float cos_theta_o = absCosTheta(wo);
    float cos_theta_i = absCosTheta(wi);

    vec3 fresnel = fresnelSchlick(wm, wo, albedo, metallic);
    vec3 specular = D(wm, roughness) * fresnel * G(wo, wi, roughness) / (4 * cos_theta_o * cos_theta_i);

    vec3 specularFraction = fresnel;
    vec3 diffuseFraction = vec3(1.0) - specularFraction;
    diffuseFraction *= 1.0 - metallic;

    result.pdf = normalDistributionPDF(wo, wm, roughness) / (4 * abs(dot(wo, wm)));
    result.f = diffuseFraction * albedo / PI + specular;
    result.wi = wi;

    return result;
}