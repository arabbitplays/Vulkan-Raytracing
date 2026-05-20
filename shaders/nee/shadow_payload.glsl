#ifndef SHADOW_PAYLOAD_GLSL
#define SHADOW_PAYLOAD_GLSL

struct ShadowPayload {
    uvec4 rng_state;
    vec3 transmittance;
    float dist_to_light;
    vec3 next_origin;
    vec3 direction;
    int current_volume_idx;
};

#endif
