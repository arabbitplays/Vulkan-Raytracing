#ifndef MLMC_CONTEXT_GLSL
#define MLMC_CONTEXT_GLSL

#include "../metalRough/options.glsl"
#include "../metalRough/payload.glsl"
#include "../metalRough/evaluation_options.glsl"

// --- MLMC-method dispatch --------------------------------------------------
// The MLMC method is a specialization constant, so the compiler folds these
// helpers to a single code path per pipeline variant. Shared between the
// closest-hit (which fills VertexResult) and the streaming reduce in raygen
// (which may need to redo the biased branch across a skipped gap).

bool pathIsSampledBiased(SamplingOptions opts) {
    if (SPEC_MLMC_METHOD == 4 || SPEC_MLMC_METHOD == 5) {
        return opts.assume_homogenous;
    } else if (SPEC_MLMC_METHOD == 6 || SPEC_MLMC_METHOD == 7 || SPEC_MLMC_METHOD == 8) {
        return opts.use_first_order_similarity;
    } else if (SPEC_MLMC_METHOD == 1 || SPEC_MLMC_METHOD == 2 || SPEC_MLMC_METHOD == 3) {
        return opts.similarity_relation;
    }
    return false;
}

void setEvalOptsBiased(inout EvaluationOptions opts, bool biased) {
    if (SPEC_MLMC_METHOD == 4 || SPEC_MLMC_METHOD == 5) {
        opts.assume_homogenous = biased;
        opts.use_similarity_relation = false;
        opts.use_first_order_similarity = false;
    } else if (SPEC_MLMC_METHOD == 6 || SPEC_MLMC_METHOD == 7 || SPEC_MLMC_METHOD == 8) {
        opts.use_similarity_relation = false;
        opts.use_first_order_similarity = biased;
        opts.assume_homogenous = false;
    } else if (SPEC_MLMC_METHOD == 1 || SPEC_MLMC_METHOD == 2 || SPEC_MLMC_METHOD == 3) {
        opts.use_similarity_relation = biased;
        opts.use_first_order_similarity = false;
        opts.assume_homogenous = false;
    }
}

// True when the current MLMC method uses deterministic-skip correlation, so
// the streaming reduce needs to buffer a 1-vertex lookahead and rebuild
// biased throughput across skipped gaps.
bool mlmcUsesSkipDeterministic() {
    return SPEC_MLMC_METHOD == 3 || SPEC_MLMC_METHOD == 8;
}

// True when the current MLMC method uses resample correlation, i.e. the diff
// draws independent biased/unbiased samples. In that mode the biased estimator
// honors the configured tracking method so the biased-only variance matches
// the biased side of the diff; same_path / skip_deterministic instead force
// regular tracking to share one path across both sides.
bool mlmcUsesResample() {
    return SPEC_MLMC_METHOD == 1 || SPEC_MLMC_METHOD == 4 || SPEC_MLMC_METHOD == 6;
}

#endif
