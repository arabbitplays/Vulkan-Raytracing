#ifndef PATH_GLSL
#define PATH_GLSL

#include "path_vertex.glsl"

// Upper bound on how many bounces a single path may take. Kept as a
// specialization constant for compatibility with the raygen dispatch code
// (MetalRoughMaterial::buildPipelines sets constant_id 0 and rebuilds the
// pipeline when recursion_depth grows beyond it). The streaming reduce
// doesn't allocate any per-vertex storage, so this only bounds the sampling
// while loop below.
layout(constant_id = 0) const uint MAX_PATH_LENGTH = 32;

// Streaming per-branch reduce state. Each pixel keeps only running scalars
// plus a 1-vertex pending slot for the shouldSkip lookahead, instead of the
// old MAX_PATH_LENGTH-sized vertex + segment arrays.
struct PathState {
    vec3 origin;
    uint len; // number of finalized vertices

    // Running per-branch accumulators.
    vec3 unbiased_light;
    vec3 unbiased_beta;
    vec3 biased_light;
    vec3 biased_beta;

    // Snapshot of unbiased_light captured when the reduce reaches
    // truncated_at vertices. Used by path_length prefix-diff MLMC to
    // evaluate the same path stream at biased and unbiased depths.
    vec3 truncated_light;
    uint truncated_at;

    // 1-vertex lookahead for skip-determinism: pending is finalized only
    // once the NEXT vertex has arrived (or we hit the end of the path),
    // so shouldSkip's peek at pending's neighbor is available.
    bool pending_valid;
    VertexResult pending;

    // Skip-determinism redo state: on skip, biased state stays frozen; on
    // the next kept vertex we rebuild the biased pre/post beta across the
    // gap from last_kept to that vertex. biased_beta_at_last_kept is the
    // biased throughput right after last_kept's contribution add (before
    // its outgoing post-beta was applied), i.e. the snapshot the redo
    // restarts from.
    bool skipped_since_last_kept;
    vec3 biased_beta_at_last_kept;
    bool last_kept_valid;
    int last_kept_type;
    int last_kept_volume_idx;
    vec3 last_kept_P;
    vec3 last_kept_V;
    vec3 last_kept_biased_bsdf;
    float last_kept_biased_dir_pdf;
    float last_kept_biased_rr_pdf;
};
PathState path_state;

void initPathState(vec3 origin) {
    path_state.origin = origin;
    path_state.len = 0;
    path_state.unbiased_light = vec3(0);
    path_state.unbiased_beta = vec3(1);
    path_state.biased_light = vec3(0);
    path_state.biased_beta = vec3(1);
    path_state.truncated_light = vec3(0);
    path_state.truncated_at = 0xffffffffu;
    path_state.pending_valid = false;
    path_state.skipped_since_last_kept = false;
    path_state.biased_beta_at_last_kept = vec3(1);
    path_state.last_kept_valid = false;
    path_state.last_kept_type = INVALID_TYPE;
    path_state.last_kept_volume_idx = -1;
}

#endif
