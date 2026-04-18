#ifndef PATH_SAMPLE_GLSL
#define PATH_SAMPLE_GLSL

#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "../common/payload.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadEXT Payload payload;

void initPayload(vec3 origin, vec3 direction) {
    payload.next_origin = origin;
    payload.next_direction = direction;
    payload.next_distance = INFINITY;
    payload.light = vec3(0.0);
    payload.depth = 0;
    payload.beta = vec3(1.0);
    payload.eta_scale = 1;
    payload.specular_bounce = false;
    payload.current_volume_idx = -1;
}

void continuePath() {
    float tmin = EPSILON;
    float tmax = payload.next_distance - EPSILON;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, payload.next_origin, tmin, payload.next_direction, tmax, 0);

    payload.depth++;
}

vec3 takeSample(uint max_depth) {
    while (payload.depth < max_depth && payload.next_direction != vec3(0.0) && payload.next_distance > 0) {
        continuePath();
    }
    return payload.light;
}


#endif
