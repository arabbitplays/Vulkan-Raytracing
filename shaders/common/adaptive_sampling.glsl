#ifndef ADAPTIVE_SAMPLING_GLSL
#define ADAPTIVE_SAMPLING_GLSL

// 95% normal-approximation confidence interval half-width; stop when the
// interval is smaller than max(rel_error * |mean|, abs_floor).
//
// `m2` is Welford's sum of squared deviations from the mean, i.e.
//   M2 = sum_i (x_i - mean_N)^2
// so the Bessel-corrected sample variance is M2 / (N - 1) and the standard
// error of the mean is sqrt(M2 / (N * (N - 1))).
#define ADAPTIVE_SAMPLING_Z 1.96

bool stop_sampling(float mean, float m2, uint sample_count,
                   uint min_samples, float rel_error, float abs_floor) {
    if (sample_count < max(min_samples, 2u)) return false;

    float n = float(sample_count);
    float std_error = sqrt(m2 / (n * (n - 1.0)));
    float half_width = ADAPTIVE_SAMPLING_Z * std_error;

    float threshold = max(rel_error * abs(mean), abs_floor);
    return half_width < threshold;
}

#endif
