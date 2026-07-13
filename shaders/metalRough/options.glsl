#ifndef OPTIONS_GLSL
#define OPTIONS_GLSL
layout(push_constant) uniform PushConstants {
    int recursion_depth;
    bool normal_mapping;
    bool sample_light;
    bool sample_bsdf;
    bool russian_roulette;
    bool similarity_relation;

    bool debug_depth;
    bool adaptive_sampling;
    bool debug_variance;
    bool debug_diff_variance;
    bool debug_adaptive_sampling;
    float adaptive_error_bound;
    uint adaptive_min_samples;

    uint accumulated_frame_count;
    uint accumulated_diff_frame_count;
    uint samples_per_pixel;
    uint diff_samples_per_pixel;

    bool do_mlmc;
    uint mlmc_method;
    uint biased_path_length;
} options;
#endif // OPTIONS_GLSL
