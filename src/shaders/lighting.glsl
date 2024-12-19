#define EPSILON 0.005

struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    vec3 reflection;
    vec3 transmission;
    float n;
    vec3 eta;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 1) rayPayloadEXT bool isShadowed;

vec3 evaluatePhong(vec3 P, vec3 N, vec3 L, float incoming_light, float dist_to_light, vec3 V, Material mat) {
    float NdotL = dot(N, L);

    vec3 diffuse = vec3(0);
    vec3 specular = vec3(0);

    if (NdotL > 0) {
        float tmin = 0.001;
        float tmax = dist_to_light;
        vec3 origin = P + EPSILON * N;
        vec3 direction = L;
        uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;

        traceRayEXT(topLevelAS, flags, 0xff, 0, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);

        if (!isShadowed) {
            diffuse = incoming_light * mat.diffuse * max(0, NdotL);

            vec3 R = reflect(-L, N);
            float VdotR = dot(V, R);
            specular = incoming_light * mat.specular * pow(max(0, VdotR), mat.n);
        }
    }

    vec3 ambient = incoming_light * mat.ambient;

    return diffuse + specular + ambient;
}

void evaluateReflection(vec3 P, vec3 N, vec3 V, Material material) {

    vec3 origin = P + EPSILON * N;
    vec3 direction = reflect(-V, N);

    float tmin = 0.01;
    float tmax = 10000.0;

    payload.depth++;
    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);
}

void evaluateTransmission(vec3 P, vec3 N, vec3 V, Material material) {
    float NdotV = dot(N, V);

    float eta = material.eta.x;
    if (NdotV < 0.0) {
        N = -N;
        NdotV = -NdotV;
    } else {
        eta = 1.0 / eta;
    }

    const float radicand = 1.0 - eta * eta * (1.0 - NdotV * NdotV);
    if (radicand < 0.0) {
        payload.direct_light = vec3(0.0);
        return;
    }

    vec3 refract_dir = normalize((eta * NdotV - sqrt(radicand)) * N - eta * V);
    vec3 origin = P + EPSILON * refract_dir;

    float tmin = 0.01;
    float tmax = 10000.0;

    payload.depth++;
    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, refract_dir, tmax, 0);
}