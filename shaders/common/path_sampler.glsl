#ifndef PATH_SAMPLE_GLSL
#define PATH_SAMPLE_GLSL

#include "../common/random.glsl"
#include "../common/constants.glsl"
#include "payload.glsl"
#include "../common/path.glsl"
#include "./options.glsl"
#include "../mlmc/mlmc_context.glsl"
#include "vertex_evaluator.glsl"
#include "path_evaluator.glsl"

layout(location = 0) rayPayloadEXT Payload payload;

void initPayload(vec3 origin, vec3 direction) {
    payload.next_vertex.P = origin;
    payload.next_vertex.meta = packVertexMeta(0u, -1, INVALID_TYPE, false);
    payload.next_segment = createNewSegment();
    payload.next_result = createNewVertexResult();
    // Fork eval_rng from the sampling stream once at path start. Chit uses only
    // eval_rng_state for NEE, so subsequent sampling of this path stays bit
    // identical to what it was before evaluation moved into the hit shader.
    payload.eval_rng_state = payload.rng_state;
    payload.eval_flags = 0u;
    payload.next_dir = direction;
    payload.depth = 0;
    payload.rr_beta = vec3(1.0);
    payload.eta_scale = 1;
    SamplingOptions sample_options;
    sample_options.similarity_relation = options.similarity_relation;
    sample_options.use_first_order_similarity = options.use_first_order_similarity;
    sample_options.assume_homogenous = options.assume_homogenous;
    sample_options.regular_tracking = options.regular_tracking;
    payload.sampling_options = sample_options;
}

bool evalRussianRoulette(vec3 beta, float eta_scale, out float pdf, inout uvec4 rng_state) {
    pdf = 1.0;
    vec3 rr_beta = beta * eta_scale;
    float beta_max_component = max(rr_beta.x, max(rr_beta.y, rr_beta.z));
    if (beta_max_component < 1) {
        float q = max(0, 1 - beta_max_component);
        float u = stepAndOutputRNGFloat(rng_state);
        if (u < q) {
            return true;
        }
        pdf = 1 - q;
    }
    return false;
}

void continuePath() {
    float tmin = EPSILON;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, payload.next_vertex.P, tmin, payload.next_dir, INFINITY, 0);

    payload.rr_beta *= getPreEvaluationBeta(payload.next_segment);
    float rr_pdf = 1;
    int next_vertex_type = getVertexType(payload.next_vertex);
    bool apply_russian_roulette = SPEC_RUSSIAN_ROULETTE
        && payload.depth > 1
        && (next_vertex_type == SURFACE_TYPE || next_vertex_type == VOLUME_TYPE);
    if (apply_russian_roulette) {
        if (evalRussianRoulette(payload.rr_beta, payload.eta_scale, rr_pdf, payload.rng_state)) {
            payload.rr_beta = vec3(0);
        } else {
            payload.next_segment.rr_pdf *= rr_pdf;
            payload.next_result.biased_rr_pdf *= rr_pdf;
            // The RR decision happens after chit's fillVertexResult, so the
            // already-baked post_beta = bsdf / dir_pdf / rr_pdf carries the
            // pre-RR rr_pdf==1. Divide out to fold in the survival factor;
            // finalizePending consumes these directly and never touches
            // biased_rr_pdf itself except in the skip-redo path.
            float inv_rr = 1.0 / rr_pdf;
            payload.next_result.biased_post_beta   *= inv_rr;
            payload.next_result.unbiased_post_beta *= inv_rr;
        }
    }
    payload.rr_beta *= getPostEvaluationBeta(payload.next_segment);

    if (next_vertex_type != INVALID_TYPE) {
        payload.depth++;
    }
}

// --- streaming reduce ------------------------------------------------------
// finalizePending consumes path_state.pending after the next vertex's type is
// known, so shouldSkip can peek at the neighbor.

bool shouldSkipPending(int next_type, bool is_last) {
    if (!mlmcUsesSkipDeterministic()) return false;
    if (path_state.len == 0) return false;         // first-vertex short-circuit
    if (is_last) return false;                     // last-vertex short-circuit
    if (path_state.pending.type != VOLUME_TYPE) return false;
    if (path_state.skipped_since_last_kept) return false;
    if (!path_state.last_kept_valid) return false;
    if (path_state.last_kept_type == VOLUME_BOUNDARY_TYPE) return false;
    if (next_type == VOLUME_BOUNDARY_TYPE) return false;
    return true;
}

// Rebuilds biased pre/post beta across a gap of skipped vertices, mirroring
// updateSimilarityBiasedThroughput but sourcing state from path_state's
// last_kept snapshot instead of a per-vertex array.
void applyBiasedRedo(VertexResult r, inout uvec4 rng_state) {
    path_state.biased_beta = path_state.biased_beta_at_last_kept;

    EvaluationOptions bopts = getUserOptions();
    setEvalOptsBiased(bopts, true);

    vec3 new_outgoing_dir = normalize(r.P - path_state.last_kept_P);

    vec3 redone_last_bsdf;
    if (path_state.last_kept_type == VOLUME_TYPE) {
        PathVertex fake_last = createNewPathVertex();
        fake_last.type = path_state.last_kept_type;
        fake_last.P = path_state.last_kept_P;
        fake_last.V = path_state.last_kept_V;
        fake_last.volume_idx = path_state.last_kept_volume_idx;
        EvaluatedVolume vv = evaluateVertexVolume(fake_last,
                                                  bopts.use_similarity_relation,
                                                  bopts.use_first_order_similarity,
                                                  bopts.assume_homogenous);
        float phase;
        if (bopts.use_first_order_similarity) {
            phase = INV_4_PI;
        } else if (bopts.use_similarity_relation) {
            phase = evaluateAlteredPhaseFunctionIdx(path_state.last_kept_V, new_outgoing_dir, vv.similarity_idx);
        } else {
            phase = henyeyGreenstein(path_state.last_kept_V, new_outgoing_dir, vv.g);
        }
        redone_last_bsdf = vv.scattering * phase;
    } else {
        // Surface last_kept: the current MLMC estimator reuses the sampled
        // biased_bsdf value across the skipped gap unchanged.
        redone_last_bsdf = path_state.last_kept_biased_bsdf;
    }
    path_state.biased_beta *= redone_last_bsdf
                            / path_state.last_kept_biased_dir_pdf
                            / path_state.last_kept_biased_rr_pdf;

    vec3 new_transmittance = vec3(1);
    if (path_state.last_kept_volume_idx >= 0
        && (r.type == VOLUME_TYPE
            || (r.type == VOLUME_BOUNDARY_TYPE && path_state.last_kept_type == VOLUME_TYPE))) {
        VolumeInstance vol = getVolume(path_state.last_kept_volume_idx);
        new_transmittance = estimateSegmentTransmittance(path_state.last_kept_P, r.P, vol,
                                                        bopts.use_similarity_relation,
                                                        bopts.use_first_order_similarity,
                                                        bopts.assume_homogenous,
                                                        rng_state);
    }
    // biased_pdf in updateSimilarityBiasedThroughput uses seg[curr].dist_pdf,
    // i.e. the sampled distance density for the current vertex.
    path_state.biased_beta *= new_transmittance / r.dist_pdf;
}

void finalizePending(int next_type, bool is_last, inout uvec4 rng_state) {
    if (!path_state.pending_valid) return;
    VertexResult r = path_state.pending;
    bool skip = shouldSkipPending(next_type, is_last);

    // Unbiased side always accumulates -- skips are transparent to it.
    path_state.unbiased_beta *= r.unbiased_pre_beta;
    path_state.unbiased_light += path_state.unbiased_beta * r.unbiased_contribution;
    path_state.unbiased_beta *= r.unbiased_post_beta;

    // Capture the snapshot right after folding in the N-th finalized vertex,
    // i.e. once unbiased_light covers indices 0 .. truncated_at-1.
    if (path_state.len + 1u == path_state.truncated_at) {
        path_state.truncated_light = path_state.unbiased_light;
    }

    if (!skip) {
        if (path_state.skipped_since_last_kept) {
            applyBiasedRedo(r, rng_state);
        } else {
            path_state.biased_beta *= r.biased_pre_beta;
        }
        path_state.biased_light += path_state.biased_beta * r.biased_contribution;
        path_state.biased_beta_at_last_kept = path_state.biased_beta;
        path_state.biased_beta *= r.biased_post_beta;

        path_state.last_kept_type = r.type;
        path_state.last_kept_volume_idx = r.volume_idx;
        path_state.last_kept_P = r.P;
        path_state.last_kept_V = r.V;
        path_state.last_kept_biased_bsdf = r.biased_bsdf;
        path_state.last_kept_biased_dir_pdf = r.biased_dir_pdf;
        path_state.last_kept_biased_rr_pdf = r.biased_rr_pdf;
        path_state.last_kept_valid = true;
        path_state.skipped_since_last_kept = false;
    } else {
        path_state.skipped_since_last_kept = true;
    }

    path_state.len++;
    path_state.pending_valid = false;
}

void takePathSample(uint max_depth, uint truncated_at) {
    max_depth = min(max_depth, MAX_PATH_LENGTH);
    initPathState(payload.next_vertex.P);
    path_state.truncated_at = truncated_at;
    while (payload.depth < max_depth && payload.next_dir != vec3(0.0) && dot(payload.rr_beta, payload.rr_beta) > 0.0) {
        continuePath();
        // Finalize the previous pending against the freshly arrived vertex's
        // type -- the 1-vertex lookahead the skip check needs. Skip-redo
        // marches transmittance through the volume, which is evaluation
        // work; use the eval rng so sampling stays bit-identical to the
        // pre-refactor stream.
        finalizePending(payload.next_result.type, /*is_last=*/ false, payload.eval_rng_state);
        // Buffer the current vertex until the next iteration decides skip.
        path_state.pending = payload.next_result;
        path_state.pending_valid = true;
    }
    // Drain the last buffered vertex; no successor -> is_last=true.
    finalizePending(INVALID_TYPE, /*is_last=*/ true, payload.eval_rng_state);
    // If sampling ended before hitting truncated_at, the truncation snapshot
    // is the full unbiased_light -- matching the old evaluate loop which
    // capped at min(evaluation_depth, path.len) for both sides.
    if (path_state.len < path_state.truncated_at) {
        path_state.truncated_light = path_state.unbiased_light;
    }
}

void takePathSample(uint max_depth) {
    takePathSample(max_depth, 0xffffffffu);
}

#endif
