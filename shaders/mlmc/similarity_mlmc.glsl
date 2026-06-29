#ifndef SIMILARITY_MLMC_GLSL
#define SIMILARITY_MLMC_GLSL

#include "../common/path_sampler.glsl"

vec3 similarityMlmc(uint sample_count, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;
    eval_options.use_similarity_relation = true;

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.similarity_relation = true;

        takePathSample(unbiased_path_length);
        color += evaluatePath(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

vec3 evaluateDiffVertex(PathVertex vertex, vec3 biased_throughput, vec3 unbiased_throughput, vec3 path_pdf, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    uvec4 rng = rng_state;

    options.use_similarity_relation = false;
    vec3 unbiased = unbiased_throughput / path_pdf * evaluateVertex(vertex, options, context, rng_state);

    rng_state = rng;
    options.use_similarity_relation = true;
    vec3 biased = biased_throughput / path_pdf * evaluateVertex(vertex, options, context, rng_state);

    return unbiased - biased;
}

// for this, the phase of the last vertex needs to be evaluated here (since the next vertex was not clear there)
// also the transmittance / visibility needs to be recalculated to match the similarity relation parameters
void updateBiasedThroughput(int curr_vertex_idx, int last_vertex_idx, inout vec3 biased_throughput, inout uvec4 rng_state) {
    PathVertex vertex = path.vertices[curr_vertex_idx];
    PathVertex last_vertex = path.vertices[last_vertex_idx];

    vec3 biased_transmittance = vec3(1);;
    if (vertex.type == VOLUME_TYPE // if this is a volume vertex
            || (vertex.type == VOLUME_BOUNDARY_TYPE && last_vertex.type == VOLUME_TYPE)) { // or an exiting volume boundary vertex
        vec3 dir = vertex.P - last_vertex.P;
        biased_transmittance = estimateTransmittance(last_vertex.P, normalize(dir), length(dir), last_vertex.volume_idx, true, rng_state);
        biased_throughput *= biased_transmittance;
    }

    // multiply the bsdf of the last evaluated vertex
    vec3 biased_bsdf = path.segments[last_vertex_idx].bsdf;
    if (last_vertex.type == VOLUME_TYPE) {
        EvaluatedVolume last_volume = evaluateVertexVolume(last_vertex, true);
        vec3 wi = normalize(vertex.P - last_vertex.P);
        float phase = evaluateAlteredPhaseFunction(last_vertex.V, wi, last_volume.g);
        biased_bsdf = last_volume.scattering * phase;
    }
    biased_throughput *= biased_bsdf;
}

vec3 similarityEvaluateCorrelatedPaths(EvaluationOptions options, inout uvec4 rng_state) {
    EvaluationContext context;
    vec3 diff = vec3(0);
    vec3 biased_throughput = vec3(1);
    vec3 unbiased_throughput = vec3(1);
    vec3 path_pdf = vec3(1);
    context.specular_bounce = false;

    PathVertex last_vertex;
    int last_vertex_idx = 0;
    int skipped_vertices = 0;

    uint evaluation_depth = min(options.evaluation_depth, path.len);
    for (int i = 0; i < evaluation_depth; i++) {
        PathVertex vertex = path.vertices[i];
        SampledSegment seg = path.segments[i];
        bool skip_vertex = false;

        if (i != 0 && vertex.type == VOLUME_TYPE) {
            if (last_vertex.type == VOLUME_TYPE && skipped_vertices == 0) {
                skipped_vertices++;
                skip_vertex = true;
            }
        }

        if (i != 0 && !skip_vertex) { // skip the first one here, since the bsdf is applied later and the transmittance is 1 anyway
            updateBiasedThroughput(i, last_vertex_idx, biased_throughput, rng_state);
        }

        context.depth = i;
        unbiased_throughput *= seg.transmittance;
        path_pdf *= 1.0 / seg.dist_pdf / seg.delta_pdf;

        if (!skip_vertex) {
            diff += evaluateDiffVertex(vertex, biased_throughput, unbiased_throughput, path_pdf, options, context, rng_state);
            skipped_vertices = 0;

            last_vertex = vertex;
            last_vertex_idx = i;
        }

        unbiased_throughput *= seg.bsdf;
        path_pdf *= 1.0 / seg.dir_pdf / seg.rr_pdf;
    }

    return diff;
}

vec3 similarityDiffMlmc(uint sample_count, uint unbiased_path_length, bool resample_path) {
    if (!resample_path) {
        vec3 diff = vec3(0);
        for (int i = 0; i < sample_count; i++) {
            ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
            initPayload(view_ray.origin, view_ray.direction);
            payload.sampling_options.similarity_relation = false;
            takePathSample(unbiased_path_length);
            diff += similarityEvaluateCorrelatedPaths(getUserOptions(), payload.rng_state);
        }
        return diff / sample_count;
    }

    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;

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

        if (resample_path) {
            payload.rng_state = sampling_rng;
            initPayload(view_ray.origin, view_ray.direction);
            payload.sampling_options.similarity_relation = true;
            takePathSample(unbiased_path_length);
        }

        payload.rng_state = eval_rng;
        eval_options.use_similarity_relation = true;
        vec3 biased_color = evaluatePath(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}


#endif
