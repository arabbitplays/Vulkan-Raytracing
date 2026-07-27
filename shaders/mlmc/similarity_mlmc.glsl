#ifndef SIMILARITY_MLMC_GLSL
#define SIMILARITY_MLMC_GLSL

#include "../common/path_sampler.glsl"
#include "../common/luminance.glsl"
#include "correlation_modes.glsl"

vec3 similarityMlmc(uint sample_count, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;
    eval_options.use_similarity_relation = true;
    eval_options.regular_tracking = true;

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.similarity_relation = true;
        payload.sampling_options.regular_tracking = true;

        takePathSample(unbiased_path_length);
        color += evaluatePath(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

void evaluateSimilarityDiffVertex(PathVertex vertex, out vec3 biased_contribution, out vec3 unbiased_contribution, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    uvec4 rng = rng_state;

    options.use_similarity_relation = false;
    unbiased_contribution = evaluateVertex(vertex, options, context, rng_state);

    rng_state = rng;
    options.use_similarity_relation = true;
    biased_contribution = evaluateVertex(vertex, options, context, rng_state);
}

// for this, the phase of the last vertex needs to be evaluated here (since the next vertex was not clear there)
// also the transmittance / visibility needs to be recalculated to match the similarity relation parameters
void updateSimilarityBiasedThroughput(int curr_vertex_idx, int last_vertex_idx, inout vec3 biased_throughput, EvaluationOptions options, inout uvec4 rng_state) {
    PathVertex vertex = getPathVertex(curr_vertex_idx);
    PathVertex last_vertex = getPathVertex(last_vertex_idx);

    vec3 biased_transmittance = vec3(1);;
    if (vertex.type == VOLUME_TYPE // if this is a volume vertex
            || (vertex.type == VOLUME_BOUNDARY_TYPE && last_vertex.type == VOLUME_TYPE)) { // or an exiting volume boundary vertex
        // the segment lies inside last_vertex's volume, so march it directly
        // instead of re-tracing rays against boundaries the path already found
        VolumeInstance volume_instance = getVolume(last_vertex.volume_idx);
        biased_transmittance = estimateSegmentTransmittance(last_vertex.P, vertex.P, volume_instance, true, options.assume_homogenous, options.regular_tracking, rng_state);
        biased_throughput *= biased_transmittance;
    }

    // multiply the bsdf of the last evaluated vertex
    vec3 biased_bsdf = path.segments[last_vertex_idx].bsdf;
    if (last_vertex.type == VOLUME_TYPE) {
        EvaluatedVolume last_volume = evaluateVertexVolume(last_vertex, true, options.assume_homogenous);
        vec3 wi = normalize(vertex.P - last_vertex.P);
        float phase = evaluateAlteredPhaseFunctionIdx(last_vertex.V, wi, last_volume.similarity_idx);
        biased_bsdf = last_volume.scattering * phase;
    }
    biased_throughput *= biased_bsdf;
}

bool shouldSkip(uint correlation_mode, int vertex_idx, int last_vertex_idx, out float pdf, inout uvec4 rng_state) {
    pdf = 1;
    if (vertex_idx == 0 || vertex_idx == path.len - 1 || getVertexType(path.vertices[vertex_idx]) != VOLUME_TYPE) {
        return false;
    }

    if (correlation_mode == SKIP_DETERMINISTIC_CORRELATION_MODE) {
        if (vertex_idx - last_vertex_idx == 1
                && getVertexType(path.vertices[last_vertex_idx]) != VOLUME_BOUNDARY_TYPE
                && getVertexType(path.vertices[vertex_idx + 1]) != VOLUME_BOUNDARY_TYPE) {
            return true;
        }
    } else if (correlation_mode == SKIP_RANDOM_CORRELATION_MODE) {
        EvaluatedVolume unbiased_vol = evaluateVertexVolume(getPathVertex(vertex_idx), false, options.assume_homogenous);
        // Keep with the biased-to-unbiased extinction ratio
        // (alpha * sigma_s + sigma_a) / (sigma_s + sigma_a),
        // using the channel that maximizes it.
        vec3 sigma_s = unbiased_vol.scattering;
        vec3 sigma_t = sigma_s + unbiased_vol.absorption;
        vec3 p_keep_rgb = (unbiased_vol.similarity_alpha * sigma_s + unbiased_vol.absorption) / max(sigma_t, vec3(1e-8));
        float p_keep = clamp(getMaxComponent(p_keep_rgb), 0.0, 1.0);
        float r = stepAndOutputRNGFloat(rng_state);
        if (r < p_keep) {
            pdf = p_keep;
            return false;
        } else {
            pdf = 1 - p_keep;
            return true;
        }
    }

    return false;
}

vec3 similarityEvaluateCorrelatedPaths(EvaluationOptions options, uint correlation_mode, inout uvec4 rng_state) {
    EvaluationContext context;
    vec3 biased_throughput = vec3(1);
    vec3 unbiased_throughput = vec3(1);
    vec3 biased_contribution = vec3(1);
    vec3 unbiased_contribution = vec3(1);
    vec3 path_pdf = vec3(1);
    context.specular_bounce = false;

    PathVertex last_vertex;
    int last_vertex_idx = 0;

    uint evaluation_depth = min(options.evaluation_depth, path.len);
    for (int i = 0; i < evaluation_depth; i++) {
        PathVertex vertex = getPathVertex(i);
        SampledSegment seg = path.segments[i];
        bool skip_vertex = false;

        float skip_pdf = 1;
        if (shouldSkip(correlation_mode, i, last_vertex_idx, skip_pdf, rng_state)) {
            skip_vertex = true;
        }
        path_pdf *= skip_pdf;

        if (i != 0 && !skip_vertex) { // skip the first one here, since the bsdf is applied later and the transmittance is 1 anyway
            updateSimilarityBiasedThroughput(i, last_vertex_idx, biased_throughput, options, rng_state);
        }

        context.depth = i;
        unbiased_throughput *= seg.transmittance;
        path_pdf *= seg.dist_pdf;

        vec3 biased = vec3(0);
        vec3 unbiased = vec3(0);
        evaluateSimilarityDiffVertex(vertex, biased, unbiased, options, context, rng_state);
        unbiased_contribution += unbiased_throughput / path_pdf * unbiased;
        if (!skip_vertex) {
            biased_contribution += biased_throughput / path_pdf * biased;

            last_vertex = vertex;
            last_vertex_idx = i;
        }

        unbiased_throughput *= seg.bsdf;
        path_pdf *= seg.dir_pdf * seg.rr_pdf;
    }

    return unbiased_contribution - biased_contribution;
}

vec3 similarityDiffMlmc(uint sample_count, uint unbiased_path_length, uint correlation_mode) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.regular_tracking = true;
    eval_options.evaluation_depth = unbiased_path_length;

    if (correlation_mode != RESAMPLE_CORRELATION_MODE) {
        vec3 diff = vec3(0);
        for (int i = 0; i < sample_count; i++) {
            ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
            initPayload(view_ray.origin, view_ray.direction);
            payload.sampling_options.similarity_relation = false;
            payload.sampling_options.regular_tracking = true;
            takePathSample(unbiased_path_length);
            diff += similarityEvaluateCorrelatedPaths(eval_options, correlation_mode, payload.rng_state);
        }
        return diff / sample_count;
    }

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);

        uvec4 sampling_rng = payload.rng_state;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.similarity_relation = false;
        takePathSample(unbiased_path_length);

        uvec4 eval_rng = payload.rng_state;
        eval_options.use_similarity_relation = false;
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        payload.rng_state = sampling_rng;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.similarity_relation = true;
        takePathSample(unbiased_path_length);

        payload.rng_state = eval_rng;
        eval_options.use_similarity_relation = true;
        vec3 biased_color = evaluatePath(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}


#endif
