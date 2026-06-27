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

vec3 evaluateDiffVertex(PathVertex vertex, vec3 biased_throughput, vec3 unbiased_throughput, EvaluationOptions options, inout EvaluationContext context, inout uvec4 rng_state) {
    uvec4 rng = rng_state;

    options.use_similarity_relation = false;
    vec3 unbiased = unbiased_throughput * evaluateVertex(vertex, options, context, rng_state);

    rng_state = rng;
    options.use_similarity_relation = true;
    vec3 biased = biased_throughput * evaluateVertex(vertex, options, context, rng_state);

    return unbiased - biased;
}

vec3 similarityEvaluateCorrelatedPaths(EvaluationOptions options, inout uvec4 rng_state) {
    EvaluationContext context;
    vec3 diff = vec3(0);
    vec3 biased_throughput = vec3(1);
    vec3 unbiased_throughput = vec3(1);
    vec3 path_pdf = vec3(1);
    context.specular_bounce = false;
    bool skip_vertex = false;

    uint evaluation_depth = min(options.evaluation_depth, path.len);
    for (int i = 0; i < evaluation_depth; i++) {
        PathVertex vertex = path.vertices[i];
        SampledSegment seg = path.segments[i];
        EvaluatedVolume volume;
        if (vertex.type == VOLUME_TYPE) {
            volume = evaluateVertexVolume(vertex, true);
        }

        vec3 biased_transmittance = seg.transmittance;
        if (seg.transmittance.x < 1 || seg.transmittance.y < 1 || seg.transmittance.z < 1) {
            if (vertex.type == VOLUME_BOUNDARY_TYPE) {
                volume = evaluateVertexVolume(path.vertices[i - 1], true);
            }
            float segment_length = i == 0 ? length(vertex.P - path.origin) : length(vertex.P - path.vertices[i - 1].P);
            biased_transmittance = vec3(transmittance(segment_length, volume.scattering.x + volume.absorption.x));
        }

        context.depth = i;
        unbiased_throughput *= seg.transmittance / seg.dist_pdf / seg.delta_pdf;
        biased_throughput *= biased_transmittance / seg.dist_pdf / seg.delta_pdf;
        diff += evaluateDiffVertex(vertex, biased_throughput, unbiased_throughput, options, context, rng_state);
        //path_pdf *= seg.dist_pdf * seg.dir_pdf;

        if (i == evaluation_depth - 1)
            break;
        vec3 biased_bsdf = seg.bsdf;
        if (vertex.type == VOLUME_TYPE) {
            vec3 wi = normalize(path.vertices[i + 1].V);
            float phase = evaluateAlteredPhaseFunction(-vertex.V, wi, volume.g);
            biased_bsdf = volume.scattering * phase;
        }

        unbiased_throughput *= seg.bsdf / seg.dir_pdf / seg.rr_pdf;
        biased_throughput *= biased_bsdf / seg.dir_pdf / seg.rr_pdf;
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
