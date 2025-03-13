float fresnel_dielectric(float cos_theta_i, float eta) {
    cos_theta_i = clamp(cos_theta_i, -1, 1);
    if (cos_theta_i < 0) {
        eta = 1 / eta;
        cos_theta_i = -cos_theta_i;
    }

    float sin2_theta_i = 1 - sqr(cos_theta_i);
    float sin2_theta_t = sin2_theta_i / sqr(eta);
    if (sin2_theta_t >= 1)
        return 1;
    float cos_theta_t = safeSqrt(1 - sin2_theta_t);

    float r_parl = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);
    float r_perp = (cos_theta_i - eta * cos_theta_t) / (cos_theta_i + eta * cos_theta_t);
    return (sqr(r_parl) + sqr(r_perp)) / 2;
}

vec3 calcDielectricBsdf(vec3 wo, vec3 wi, float roughness, float eta) {
    if (eta == 1 || effectivelySmooth(roughness, roughness)) return vec3(0);

    float cos_theta_o = cosTheta(wo);
    float cos_theta_i = cosTheta(wi);

    bool reflect = cos_theta_o * cos_theta_i > 0;
    float etap = 1;
    if (!reflect)
        etap = cos_theta_o > 0 ? eta : (1 / eta);

    vec3 wm = wi * etap + wo;
    if (cos_theta_o == 0 || cos_theta_i == 0 || lengthSquared(wm) == 0) return vec3(0);
    wm = faceForward(normalize(wm), vec3(0, 0, 1));

    if (dot(wm, wi) * cos_theta_i < 0 || dot(wm, wo) * cos_theta_o < 0)
        return vec3(0);

    float fresnel = fresnel_dielectric(dot(wo, wm), eta);
    if (reflect) {
        // compute reflection
        return vec3(D(wm, roughness, roughness) * fresnel * G(wo, wi, roughness, roughness) / abs(4 * cos_theta_o * cos_theta_i));
    } else {
        // compute transmission
        float denom = sqr(dot(wi, wm) + dot(wo, wm) / etap) * cos_theta_o * cos_theta_i;
        float ft = D(wm, roughness, roughness) * (1 - fresnel) * G(wo, wi, roughness, roughness) * abs(dot(wi, wm) * dot(wo, wm) / denom);
        ft /= sqr(etap);
        return vec3(ft);
    }
}

bool refract(vec3 wi, vec3 n, float eta, inout float etap, inout vec3 wt) {
    float cos_theta_i = dot(n, wi);
    if (cos_theta_i < 0) {
        eta = 1 / eta;
        cos_theta_i = -cos_theta_i;
        n = -n;
    }

    float sin2_theta_i = max(0, 1 - sqr(cos_theta_i));
    float sin2_theta_t = sin2_theta_i / sqr(eta);
    if (sin2_theta_t >= 1) {
        return false;
    }
    float cos_theta_t = safeSqrt(1 - sin2_theta_t);

    wt = -wi / eta + (cos_theta_i / eta - cos_theta_t) * n;
    etap = eta;
    return true;
}

BsdfSample sampleDielectricBsdf(vec3 wo, float roughness, float eta, inout uvec4 rngState) {
    BsdfSample result = BsdfSample(vec3(0.0), vec3(0.0), 1, false);

    if (eta == 1 || effectivelySmooth(roughness, roughness)) {
        float R = fresnel_dielectric(cosTheta(wo), eta);
        float T = 1 - R;

        float u = stepAndOutputRNGFloat(rngState);

        if (u < R / (R + T)) {
            // sample perfect specular BRDF
            vec3 wi = vec3(-wo.x, -wo.y, wo.z);
            result.f = vec3(R / absCosTheta(wi));
            result.wi = wi;
            result.pdf = R / (R + T);
            result.isSpecular = true;
            return result;
        } else {
            // sample perfect specular BTDF
            vec3 wi;
            float etap = 1;
            bool valid = refract(wo, vec3(0, 0, 1), eta, etap, wi);
            if (!valid) return result;

            vec3 ft = vec3(T / absCosTheta(wi));
            ft /= sqr(etap);

            result.f = ft;
            result.wi = wi;
            result.pdf = T / (R + T);
            result.isSpecular = true;
            return result;
        }
    }

    return result;
}