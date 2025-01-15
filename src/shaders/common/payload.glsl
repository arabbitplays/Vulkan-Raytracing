struct Payload {
    vec3 light;
    int depth;
    uint rng_state;
    vec3 next_direction;
    vec3 next_origin;
    vec3 contribution;
};