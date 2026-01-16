struct Payload {
    vec3 light;
    int depth;
    uvec4 rng_state;
    vec3 next_direction;
    vec3 next_origin;
    vec3 beta;
    float eta_scale; // used for russian roulette
    bool specular_bounce;
};