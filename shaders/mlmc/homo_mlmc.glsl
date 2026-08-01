#ifndef HOMO_MLMC_GLSL
#define HOMO_MLMC_GLSL

#include "../common/path_sampler.glsl"
#include "../common/luminance.glsl"
#include "correlation_modes.glsl"

vec3 homoMlmc(uint sample_count, uint unbiased_path_length) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;
    eval_options.assume_homogenous = true;

    vec3 color = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = true;

        takePathSample(unbiased_path_length);
        color += evaluatePath(eval_options, payload.rng_state);
    }
    color /= sample_count;
    return color;
}

vec3 evaluateHomoDiffVertex(PathVertex vertex, vec3 biased_throughput, vec3 unbiased_throughput, vec3 path_pdf, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    uvec4 rng = rng_state;

    options.assume_homogenous = false;
    vec3 unbiased = unbiased_throughput / path_pdf * evaluateVertex(vertex, options, context, rng_state);

    rng_state = rng;
    options.assume_homogenous = true;
    vec3 biased = biased_throughput / path_pdf * evaluateVertex(vertex, options, context, rng_state);

    return unbiased - biased;
}

void updateHomoUnbiasedTransmittance(int curr_vertex_idx, inout vec3 unbiased_throughput, EvaluationOptions options, inout uvec4 rng_state) {
    if (curr_vertex_idx <= 0)
        return;

    PathVertex vertex = getPathVertex(curr_vertex_idx);
    PathVertex last_vertex = getPathVertex(curr_vertex_idx - 1);

    if (vertex.type == VOLUME_TYPE // if this is a volume vertex
            || (vertex.type == VOLUME_BOUNDARY_TYPE && last_vertex.type == VOLUME_TYPE)) { // or an exiting volume boundary vertex
        VolumeInstance volume_instance = getVolume(last_vertex.volume_idx);
        vec3 unbiased_transmittance = estimateSegmentTransmittance(last_vertex.P, vertex.P, volume_instance, options.use_similarity_relation, options.use_first_order_similarity, false, options.regular_tracking, rng_state);
        unbiased_throughput *= unbiased_transmittance;
    }
}

// Multiplies the bsdf of the previous vertex onto the unbiased throughput. The
// stored segment bsdf of a volume vertex holds the homogenized scattering
// coefficient, so it is rebuilt from the local volume lookup; surface bsdfs are
// identical on both sides and pass through unchanged.
void updateHomoUnbiasedBrdf(int curr_vertex_idx, inout vec3 unbiased_throughput, EvaluationOptions options) {
    if (curr_vertex_idx <= 0)
        return;

    PathVertex vertex = getPathVertex(curr_vertex_idx);
    PathVertex last_vertex = getPathVertex(curr_vertex_idx - 1);

    vec3 unbiased_bsdf = path.segments[curr_vertex_idx - 1].bsdf;
    if (last_vertex.type == VOLUME_TYPE) {
        EvaluatedVolume last_volume = evaluateVertexVolume(last_vertex, options.use_similarity_relation, options.use_first_order_similarity, false);
        vec3 wi = normalize(vertex.P - last_vertex.P);
        float phase;
        if (options.use_first_order_similarity) {
            phase = INV_4_PI;
        } else if (options.use_similarity_relation) {
            phase = evaluateAlteredPhaseFunctionIdx(last_vertex.V, wi, last_volume.similarity_idx);
        } else {
            phase = henyeyGreenstein(last_vertex.V, wi, last_volume.g);
        }
        unbiased_bsdf = last_volume.scattering * phase;
    }
    unbiased_throughput *= unbiased_bsdf;
}

vec3 homoEvaluateCorrelatedPaths(EvaluationOptions options, inout uvec4 rng_state) {
    EvaluationContext context;
    vec3 diff = vec3(0);
    vec3 biased_throughput = vec3(1);
    vec3 unbiased_throughput = vec3(1);
    vec3 path_pdf = vec3(1);
    context.specular_bounce = false;

    uint evaluation_depth = min(options.evaluation_depth, path.len);
    for (int i = 0; i < evaluation_depth; i++) {
        PathVertex vertex = getPathVertex(i);
        SampledSegment seg = path.segments[i];

        updateHomoUnbiasedTransmittance(i, unbiased_throughput, options, rng_state);
        updateHomoUnbiasedBrdf(i, unbiased_throughput, options);

        context.depth = i;
        // homogeneously sampled segments carry no null-scattering term
        biased_throughput *= seg.transmittance;
        path_pdf *= seg.dist_pdf * seg.delta_pdf;

        diff += evaluateHomoDiffVertex(vertex, biased_throughput, unbiased_throughput, path_pdf, options, context, rng_state);

        biased_throughput *= seg.bsdf;
        path_pdf *= seg.dir_pdf * seg.rr_pdf;
    }

    return diff;
}

vec3 homoDiffMlmc(uint sample_count, uint unbiased_path_length, uint correlation_mode) {
    EvaluationOptions eval_options = getUserOptions();
    eval_options.evaluation_depth = unbiased_path_length;

    if (correlation_mode != RESAMPLE_CORRELATION_MODE) {
        vec3 diff = vec3(0);
        for (int i = 0; i < sample_count; i++) {
            ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);
            initPayload(view_ray.origin, view_ray.direction);
            payload.sampling_options.assume_homogenous = true;
            takePathSample(unbiased_path_length);
            diff += homoEvaluateCorrelatedPaths(eval_options, payload.rng_state);
        }
        return diff / sample_count;
    }

    vec3 diff = vec3(0);
    for (int i = 0; i < sample_count; i++) {
        ViewRay view_ray = generateViewRay(vec2(gl_LaunchIDEXT.xy), vec2(gl_LaunchSizeEXT.xy), sceneData.inv_view, sceneData.inv_proj, payload.rng_state);

        uvec4 sampling_rng = payload.rng_state;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = false;
        takePathSample(unbiased_path_length);

        uvec4 eval_rng = payload.rng_state;
        eval_options.assume_homogenous = false;
        vec3 unbiased_color = evaluatePath(eval_options, payload.rng_state);

        payload.rng_state = sampling_rng;
        initPayload(view_ray.origin, view_ray.direction);
        payload.sampling_options.assume_homogenous = true;
        takePathSample(unbiased_path_length);

        payload.rng_state = eval_rng;
        eval_options.assume_homogenous = true;
        vec3 biased_color = evaluatePath(eval_options, payload.rng_state);

        diff += unbiased_color - biased_color;
    }
    diff /= sample_count;
    return diff;
}

#endif
