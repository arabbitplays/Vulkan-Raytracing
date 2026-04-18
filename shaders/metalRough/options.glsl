#ifndef OPTIONS_GLSL
#define OPTIONS_GLSL
layout(push_constant) uniform PushConstants {
    int recursion_depth;
    bool normal_mapping;
    bool sample_light;
    bool sample_bsdf;
    bool russian_roulette;
    uint accumulated_frame_count;
    uint samples_per_pixel;
    uint diff_samples_per_pixel;
    bool do_mlmc;
    uint mlmc_method;
} options;
#endif // OPTIONS_GLSL
