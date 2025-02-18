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

vec3 calcBRDF(vec3 n, vec3 v, vec3 l, vec3 albedo, float metallic, float roughness) {
    vec3 h = normalize(l + v);

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

    vec3 brdf = calcBRDF(N, V, L, albedo, metallic, roughness);
    return brdf * in_radiance * max(dot(N, L), 0.0);
}

struct LightSample {
    vec3 P;
    vec3 light;
    float pdf;
    bool same_surface; // point P is on an emissive surface -> no shadow ray
};

LightSample sampleEmittingPrimitive(vec3 P) {
    float u = stepAndOutputRNGFloat(payload.rng_state);
    uint emitting_instance_idx = min(uint(u * options.emitting_instances_count), options.emitting_instances_count - 1);
    float pmf_light = 1.0 / options.emitting_instances_count;

    EmittingInstance emitting_instance = emitting_instance_buffer.instances[emitting_instance_idx];

    u = stepAndOutputRNGFloat(payload.rng_state);
    uint primitive_idx = uint(min(u * emitting_instance.primitive_count, emitting_instance.primitive_count - 1));
    float pmf_primitive = 1.0 / emitting_instance.primitive_count;

    Triangle triangle = getTriangle(emitting_instance.instance_idx, primitive_idx);

    u = stepAndOutputRNGFloat(payload.rng_state);
    float v = stepAndOutputRNGFloat(payload.rng_state);
    if (u + v > 1.0) {
        u = 1 - u;
        v = 1 - v;
    }

    vec3 sampled_P = (1 - u - v) * triangle.A.position + u * triangle.B.position + v * triangle.C.position;
    sampled_P = vec3(emitting_instance.transform * vec4(sampled_P, 1.0));
    float area = 0.5 * length(cross(triangle.B.position - triangle.A.position, triangle.C.position - triangle.A.position));
    float pdf = 1.0 / area;

    mat3 normal_matrix = transpose(inverse(mat3(emitting_instance.transform)));
    vec3 N = normalize((1 - u - v) * triangle.A.normal + u * triangle.B.normal + v * triangle.C.normal);
    N = normalize(vec3(normal_matrix * N));

    Material material = getMaterial(triangle.material_idx);
    vec3 li = vec3(0.0);
    if (gl_InstanceCustomIndexEXT == emitting_instance.instance_idx || dot(P - sampled_P, N) > 0.0) {
        li = material.emission_color * material.emission_power;
    }

    LightSample result;
    result.P = sampled_P;
    result.light = li;
    result.pdf = pmf_light * pmf_primitive * pdf;

    if (gl_InstanceCustomIndexEXT == emitting_instance.instance_idx) {
        result.same_surface = true;
    } else {
        result.same_surface = false;
    }

    return result;
}

bool unoccluded(vec3 P, vec3 L, float distance_to_light, vec3 geom_N) {
    float tmin = 0.0;
    float tmax = distance_to_light - 2 * EPSILON;
    vec3 direction = L;
    vec3 origin = P + EPSILON * L;
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed = true;

    traceRayEXT(topLevelAS, flags, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);

    return !isShadowed;
}