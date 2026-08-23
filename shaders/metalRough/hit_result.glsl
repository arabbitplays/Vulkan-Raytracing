#ifndef HIT_RESULT_GLSL
#define HIT_RESULT_GLSL

#include "../common/path_vertex.glsl"
#include "../mlmc/mlmc_context.glsl"
#include "options.glsl"
#include "payload.glsl"
#include "vertex_evaluator.glsl"
#include "evaluation_options.glsl"

// Local re-declaration of the evaluateVertex dispatch. path_evaluator.glsl
// hosts the same function but drags in the whole-path reduce, which we don't
// want in the closest-hit translation unit.
//
// `material` / `volume` must have been evaluated under eval_opts. For the
// sampled branch the closest-hit hands them in already; the other branch
// re-evaluates the volume before calling (material is branch-independent).
vec3 evaluateVertexAt(PathVertex vertex, EvaluatedMaterial material, EvaluatedVolume volume, EvaluationOptions eval_opts, inout EvaluationContext ctx, inout uvec4 rng_state) {
    if (vertex.type == SURFACE_TYPE) {
        return evaluateSurfaceVertex(vertex, material, eval_opts, ctx, rng_state);
    } else if (vertex.type == VOLUME_TYPE) {
        return evaluateVolumeVertex(vertex, volume, eval_opts, ctx, rng_state);
    }
    return vec3(0);
}

// --- Cross-branch derivation -----------------------------------------------
// When the sampled path uses one branch's params (biased or unbiased), the
// other branch's per-vertex throughput factors are recomputed here, mirroring
// updateSimilarityBiasedThroughput / updateHomoUnbiased{Transmittance,Brdf}.

// Recomputes the transmittance across the segment entering `vertex` under
// `eval_opts`. Returns the sampled transmittance for vacuum / non-volume
// segments; matches the reachability checks used by the current MLMC combiners.
vec3 deriveOtherTransmittance(PathVertex vertex, vec3 prev_P, int prev_type, int prev_volume_idx,
                              EvaluationOptions eval_opts, inout uvec4 rng_state) {
    bool segment_in_volume = prev_volume_idx >= 0
        && (vertex.type == VOLUME_TYPE
            || (vertex.type == VOLUME_BOUNDARY_TYPE && prev_type == VOLUME_TYPE));
    if (!segment_in_volume) {
        return vec3(-1); // sentinel: fall back to sampled transmittance
    }
    VolumeInstance vol = getVolume(prev_volume_idx);
    return estimateSegmentTransmittance(prev_P, vertex.P,
                                        vol,
                                        eval_opts.use_similarity_relation,
                                        eval_opts.use_first_order_similarity,
                                        eval_opts.assume_homogenous,
                                        rng_state);
}

// Recomputes the bsdf value at `vertex` for the sampled outgoing direction
// under `eval_opts`. Surfaces are unchanged across branches; only VOLUME
// vertices carry the branch-dependent scattering coefficient and phase.
vec3 deriveOtherBsdf(PathVertex vertex, vec3 outgoing_dir, EvaluationOptions eval_opts,
                     vec3 fallback_bsdf) {
    if (vertex.type != VOLUME_TYPE) {
        return fallback_bsdf;
    }
    EvaluatedVolume vol = evaluateVertexVolume(vertex,
                                               eval_opts.use_similarity_relation,
                                               eval_opts.use_first_order_similarity,
                                               eval_opts.assume_homogenous);
    float phase;
    if (eval_opts.use_first_order_similarity) {
        phase = INV_4_PI;
    } else if (eval_opts.use_similarity_relation) {
        phase = evaluateAlteredPhaseFunctionIdx(vertex.V, outgoing_dir, vol.similarity_idx);
    } else {
        phase = henyeyGreenstein(vertex.V, outgoing_dir, vol.g);
    }
    return vol.scattering * phase;
}

// --- fillVertexResult ------------------------------------------------------
// Writes payload.next_result for the just-sampled vertex, populating whichever
// biased/unbiased branches payload.eval_flags requests. Called from the tail
// of the closest-hit main().
//
// `material` is only read when vertex.type == SURFACE_TYPE (branch-independent).
// `sampled_volume` is only read when vertex.type == VOLUME_TYPE and was
// evaluated under the sampled branch's options -- the other branch re-evaluates.
void fillVertexResult(PathVertex vertex, EvaluatedMaterial material, EvaluatedVolume sampled_volume, vec3 prev_P, int prev_type, int prev_volume_idx) {
    VertexResult r = createNewVertexResult();
    r.type = vertex.type;
    r.P = vertex.P;
    r.V = vertex.V;
    r.volume_idx = vertex.volume_idx;

    SampledSegment seg = payload.next_segment;
    r.dist_pdf = seg.dist_pdf;
    vec3 sampled_pre = getPreEvaluationBeta(seg);
    vec3 sampled_post = getPostEvaluationBeta(seg);

    bool sampled_biased = pathIsSampledBiased(payload.sampling_options);

    EvaluationOptions unbiased_opts = getUserOptions();
    setEvalOptsBiased(unbiased_opts, false);
    EvaluationOptions biased_opts = getUserOptions();
    setEvalOptsBiased(biased_opts, true);

    // Same eval-rng snapshot for both branches so a correlated diff (SAME_PATH
    // / SKIP_DETERMINISTIC) draws the same NEE light sample on both sides,
    // matching evaluate{Homo,Similarity}DiffVertex today.
    uvec4 eval_rng_snap = payload.eval_rng_state;
    EvaluationContext ctx;
    ctx.depth = payload.depth;
    ctx.specular_bounce = false;

    // Outgoing direction sampled at this vertex — needed for cross-branch bsdf.
    vec3 outgoing_dir = payload.next_dir;

    if ((payload.eval_flags & EVAL_FLAG_UNBIASED) != 0u) {
        payload.eval_rng_state = eval_rng_snap;
        // sampled_volume was evaluated under the sampled branch; re-evaluate
        // only when this branch is the "other" one.
        EvaluatedVolume unbiased_volume = sampled_volume;
        if (sampled_biased && vertex.type == VOLUME_TYPE) {
            unbiased_volume = evaluateVertexVolume(vertex, unbiased_opts.use_similarity_relation, unbiased_opts.use_first_order_similarity, unbiased_opts.assume_homogenous);
        }
        r.unbiased_contribution = evaluateVertexAt(vertex, material, unbiased_volume, unbiased_opts, ctx, payload.eval_rng_state);

        if (sampled_biased) {
            vec3 other_t = deriveOtherTransmittance(vertex, prev_P, prev_type, prev_volume_idx,
                                                    unbiased_opts, payload.eval_rng_state);
            if (other_t.x < 0.0) {
                r.unbiased_pre_beta = sampled_pre;
            } else {
                r.unbiased_pre_beta = other_t * seg.null_scattering / seg.delta_pdf / seg.dist_pdf;
            }
            vec3 other_bsdf = deriveOtherBsdf(vertex, outgoing_dir, unbiased_opts, seg.bsdf);
            r.unbiased_post_beta = other_bsdf / seg.dir_pdf / seg.rr_pdf;
        } else {
            r.unbiased_pre_beta = sampled_pre;
            r.unbiased_post_beta = sampled_post;
        }
    }

    if ((payload.eval_flags & EVAL_FLAG_BIASED) != 0u) {
        payload.eval_rng_state = eval_rng_snap;
        EvaluatedVolume biased_volume = sampled_volume;
        if (!sampled_biased && vertex.type == VOLUME_TYPE) {
            biased_volume = evaluateVertexVolume(vertex, biased_opts.use_similarity_relation, biased_opts.use_first_order_similarity, biased_opts.assume_homogenous);
        }
        r.biased_contribution = evaluateVertexAt(vertex, material, biased_volume, biased_opts, ctx, payload.eval_rng_state);

        if (!sampled_biased) {
            vec3 other_t = deriveOtherTransmittance(vertex, prev_P, prev_type, prev_volume_idx,
                                                    biased_opts, payload.eval_rng_state);
            if (other_t.x < 0.0) {
                r.biased_pre_beta = sampled_pre;
            } else {
                r.biased_pre_beta = other_t * seg.null_scattering / seg.delta_pdf / seg.dist_pdf;
            }
            vec3 other_bsdf = deriveOtherBsdf(vertex, outgoing_dir, biased_opts, seg.bsdf);
            r.biased_post_beta = other_bsdf / seg.dir_pdf / seg.rr_pdf;
            r.biased_bsdf = other_bsdf;
        } else {
            r.biased_pre_beta = sampled_pre;
            r.biased_post_beta = sampled_post;
            r.biased_bsdf = seg.bsdf;
        }
        r.biased_dir_pdf = seg.dir_pdf;
        r.biased_rr_pdf = seg.rr_pdf;
    }

    payload.next_result = r;
}

#endif
