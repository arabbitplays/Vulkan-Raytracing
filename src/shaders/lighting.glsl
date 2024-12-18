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

void evaluateReflection(vec3 P, vec3 N, vec3 V, Material mat) {

    payload.next_origin = P + EPSILON * N;
    payload.next_direction = reflect(-V, N);
    payload.next_contribution_factor = mat.reflection;
}