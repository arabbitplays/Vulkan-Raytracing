#include "../common/constants.glsl"

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
    m.eta = vec3(B.w); // TODO Fix this
    m.emission_color = C.xyz;
    m.emission_power = C.w;

    return m;
}

// Throwbridge Reitz Distribution
float D(vec3 wm, float alpha_x, float alpha_y) {
    float tan2_theta = tan2Theta(wm);
    if (isinf(tan2_theta)) return 0;
    float cos4_theta = sqr(cos2Theta(wm));
    float e = tan2_theta * (sqr(cosPhi(wm) / alpha_x) + sqr(sinPhi(wm) / alpha_y));
    return 1 / (PI * alpha_x * alpha_y * cos4_theta * sqr(e + 1));
}

float lambda(vec3 w, float alpha_x, float alpha_y) {
    float tan2_theta = tan2Theta(w);
    if (isinf(tan2_theta)) return 0;
    float alpha2 = sqr(cosPhi(w) * alpha_x) + sqr(sinPhi(w) * alpha_y);
    return (sqrt(1 + alpha2 * tan2_theta) - 1) / 2;
}

float G( vec3 wo, vec3 wi, float alpha_x, float alpha_y) {
    return 1 / (1 + lambda(wo, alpha_x, alpha_y) + lambda(wi, alpha_x, alpha_y));
}

float G1(vec3 w, float alpha_x, float alpha_y) {
    return 1 / (1 + lambda(w, alpha_x, alpha_y));
}

float D(vec3 w, vec3 wm, float alpha_x, float alpha_y) {
    return G1(w, alpha_x, alpha_y) / absCosTheta(w) * D(wm, alpha_x, alpha_y) * abs(dot(w, wm));
}

vec3 sampleWm(vec3 w, float alpha_x, float alpha_y, inout uvec4 rngState) {
    vec3 wh = normalize(vec3(alpha_x * w.x, alpha_y * w.y, w.z));
    if (wh.z < 0) wh = -wh;

    vec3 t1 = (wh.z < 0.99999) ? normalize(cross(vec3(0, 0, 1), wh)) : vec3(1, 0, 0);
    vec3 t2 = cross(wh, t1);

    vec2 p = sampleUniformDiskPolar(rngState);

    float h = sqrt(1 - sqr(p.x));
    p.y = lerp((1 + wh.z) / 2, h, p.y);

    float pz = sqrt(max(0, 1 - lengthSquared(p)));
    vec3 nh = p.x * t1 + p.y * t2 + pz * wh;

    return normalize(vec3(alpha_x * nh.x, alpha_x * nh.y, max(1.0E-6, nh.z)));
}

float normalDistributionPDF(vec3 w, vec3 wm, float alpha_x, float alpha_y) {
    return D(w, wm, alpha_x, alpha_y);
}

vec3 fresnelSchlick(float cosTheta, vec3 albedo, float metallic) {
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

bool effectivelySmooth(float alpha_x, float alpha_y) {
    return max(alpha_x, alpha_y) < 1E-3;
}