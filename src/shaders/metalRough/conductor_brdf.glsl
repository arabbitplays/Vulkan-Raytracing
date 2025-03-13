layout(location = 1) rayPayloadEXT bool isShadowed;

vec3 calcConductorBRDF(vec3 wo, vec3 wi, vec3 albedo, float metallic, float roughness) {
    if (!sameHemisphere(wo, wi) || effectivelySmooth(roughness, roughness)) return vec3(0);

    float cos_theta_o = cosTheta(wo);
    float cos_theta_i = cosTheta(wi);
    if (cos_theta_i == 0 || cos_theta_o == 0) return vec3(0);

    vec3 wm = wi + wo;
    if (lengthSquared(wm) == 0) return vec3(0);
    wm = normalize(wm);

    // torrance sparrow brdf
    vec3 fresnel = fresnelSchlick(abs(dot(wo, wm)), albedo, metallic);
    vec3 specular = D(wm, roughness, roughness) * fresnel * G(wo, wi, roughness, roughness) / (4 * cos_theta_o * cos_theta_i);

    vec3 specularFraction = fresnel;
    vec3 diffuseFraction = vec3(1.0) - specularFraction;
    diffuseFraction *= 1.0 - metallic;

    return diffuseFraction * albedo / PI + specular;
}

BsdfSample sampleConductorBrdf(vec3 wo, vec3 albedo, float metallic, float roughness, inout uvec4 rngState) {
    BsdfSample result = BsdfSample(vec3(0.0), vec3(0.0), 1, BSDF_FLAG_UNSET, 1);

    if (effectivelySmooth(roughness, roughness)) {
        vec3 wi = vec3(-wo.x, -wo.y, wo.z);
        result.f = fresnelSchlick(absCosTheta(wi), albedo, metallic) / absCosTheta(wi);
        result.pdf = 1;
        result.wi = wi;
        result.flags = BSDF_FLAG_SPECULAR_REFLECTION;
        return result;
    }

    vec3 wm = sampleWm(wo, roughness, roughness, rngState);
    vec3 wi = reflect(-wo, wm);
    if (!sameHemisphere(wo, wi)) return result;

    float cos_theta_o = absCosTheta(wo);
    float cos_theta_i = absCosTheta(wi);

    vec3 fresnel = fresnelSchlick(abs(dot(wo, wm)), albedo, metallic);
    vec3 specular = D(wm, roughness, roughness) * fresnel * G(wo, wi, roughness, roughness) / (4 * cos_theta_o * cos_theta_i);

    vec3 specularFraction = fresnel;
    vec3 diffuseFraction = vec3(1.0) - specularFraction;
    diffuseFraction *= 1.0 - metallic;

    result.pdf = normalDistributionPDF(wo, wm, roughness, roughness) / (4 * abs(dot(wo, wm)));
    result.f = diffuseFraction * albedo / PI + specular;
    result.wi = wi;
    result.flags = BSDF_FLAG_GLOSSY_REFLECTION;

    return result;
}