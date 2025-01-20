layout(push_constant) uniform PushConstants {
    int recursion_depth;
    bool shadows;
    bool fresnel;
    bool dispersion;
    bool normal_mapping;
    uint curr_sample_count;
    uint emitting_primitive_count;
} options;