layout(push_constant) uniform PushConstants {
    int recursion_depth;
    bool normal_mapping;
    bool sample_light;
    bool sample_bsdf;
    bool russian_roulette;
    uint curr_sample_count;
    uint emitting_instances_count;
    uint samples_per_pixel;
} options;