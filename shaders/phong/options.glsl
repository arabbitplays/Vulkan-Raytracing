#ifndef OPTIONS_GLSL
#define OPTIONS_GLSL
layout(push_constant) uniform PushConstants {
    int recursion_depth;
    bool shadows;
    bool fresnel;
    bool dispersion;
// TODO remove path tracing constants
    uint curr_sample_count;
    uint samples_per_pixel;
} options;
#endif // OPTIONS_GLSL
