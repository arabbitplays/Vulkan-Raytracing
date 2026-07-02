#ifndef ADAPTIVE_SAMPLING_GLSL
#define ADAPTIVE_SAMPLING_GLSL

// 95% normal-approximation confidence interval half-width; stop when the
// interval is smaller than error_bound * |mean|.
#define ADAPTIVE_SAMPLING_Z 1.96

bool stop_sampling(float first_moment, float second_moment, uint sample_count, float error_bound) {
    if (sample_count < 2) return false;
    float variance = max(second_moment - first_moment * first_moment, 0.0);
    float std_error = sqrt(variance / float(sample_count));
    float rel_error = ADAPTIVE_SAMPLING_Z * std_error / max(abs(first_moment), 1e-8);
    return rel_error < error_bound;
}

#endif
