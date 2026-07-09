#ifndef OPTIONS_GLSL
#define OPTIONS_GLSL

// Specialization constants — set at pipeline creation from
// MetalRoughMaterial::buildPipelines and MetalRoughMaterial::ensurePipelineSpecialization.
// Constant_id 0 (MAX_PATH_LENGTH) is declared in shaders/common/path.glsl.
layout(constant_id = 1) const uint SPEC_MLMC_METHOD = 0;
layout(constant_id = 2) const bool SPEC_DO_MLMC = false;
layout(constant_id = 3) const bool SPEC_SAMPLE_BSDF = false;
layout(constant_id = 4) const bool SPEC_RUSSIAN_ROULETTE = false;

layout(push_constant) uniform PushConstants {
    int recursion_depth;
    bool normal_mapping;
    bool sample_light;
    bool similarity_relation;
    bool assume_homogenous;

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

    uint biased_path_length;
} options;
#endif // OPTIONS_GLSL
