#ifndef TRANSMITTANCE_ESTIMATOR_GLSL
#define TRANSMITTANCE_ESTIMATOR_GLSL

#include "../nee/shadow_payload.glsl"
#include "../common/constants.glsl"

layout(location = 1) rayPayloadEXT ShadowPayload shadow_payload;

float transmittance(float distance, float extinction) {
    return exp(-extinction * distance);
}

vec3 estimateTransmittance(vec3 P, vec3 L, float distance_to_light, int volume_idx, bool use_similarity_relation, inout uvec4 rng_state) {
    shadow_payload.rng_state = rng_state;
    shadow_payload.transmittance = vec3(1);
    shadow_payload.dist_to_light = distance_to_light;
    shadow_payload.next_origin = P;
    shadow_payload.direction = normalize(L);
    shadow_payload.current_volume_idx = volume_idx;
    shadow_payload.similarity_relation = use_similarity_relation;

    while (length(shadow_payload.transmittance) > 0.0f && shadow_payload.dist_to_light > EPSILON) {
        float tmin = EPSILON;
        float tmax = INFINITY;
        vec3 direction = shadow_payload.direction;
        vec3 origin = shadow_payload.next_origin;
        uint flags = gl_RayFlagsOpaqueEXT;
        traceRayEXT(topLevelAS, flags, 0xff, 1, 0, 1, origin.xyz, tmin, direction.xyz, tmax, 1);
    }

    rng_state = shadow_payload.rng_state;
    return shadow_payload.transmittance;
}

vec3 estimateTransmittance(vec3 P, vec3 L, float distance_to_light, bool use_similarity_relation, inout uvec4 rng_state) {
    return estimateTransmittance(P, L, distance_to_light, -1, use_similarity_relation, rng_state);
}

#endif