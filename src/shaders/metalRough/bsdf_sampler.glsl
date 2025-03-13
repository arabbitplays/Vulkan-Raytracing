#include "bsdf_flags.glsl"

struct BsdfSample {
    vec3 f;
    vec3 wi;
    float pdf;
    int flags;
    float eta; // used for russian roulette
};

#include "trowbridge_reitz_distribution.glsl"
#include "conductor_brdf.glsl"
#include "dielectric_bsdf.glsl"

vec3 computeBsdf(vec3 wo, vec3 wi, vec3 albedo, float metallic, float roughness, float eta) {
    if (metallic == 0) {
        return calcDielectricBsdf(wo, wi, roughness, eta);
    } else {
        return calcConductorBRDF(wo, wi, albedo, metallic, roughness);
    }
}

BsdfSample sampleBsfd(vec3 wo, vec3 albedo, float metallic, float roughness, float eta, inout uvec4 rngState) {
    if (metallic == 0) {
        return sampleDielectricBsdf(wo, roughness, eta, rngState);
    } else {
        return sampleConductorBrdf(wo, albedo, metallic, roughness, rngState);
    }
}