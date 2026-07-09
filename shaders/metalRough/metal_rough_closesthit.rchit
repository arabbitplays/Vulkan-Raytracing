#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "payload.glsl"
#include "../common/scene_data.glsl"
#include "../common/layout.glsl"
#include "../common/random.glsl"

#include "options.glsl"
#include "vertex_evaluator.glsl"

#include "../volume/layout.glsl"
#include "../volume/distance_sampler.glsl"
#include "../volume/transmittance_estimator.glsl"
#include "../volume/phase_function.glsl"
#include "../similarity/altered_phase_function.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT vec3 attribs;

#include "bsdf_sampler.glsl"
#include "light_sampler.glsl"
#include "../common/path_vertex.glsl"

PathVertex createSurfaceVertex() {
    PathVertex vertex = createNewPathVertex();
    vertex.type = SURFACE_TYPE;

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    vertex.material_idx = triangle.material_idx;

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x;
    float beta = barycentricCoords.y;
    float gamma = barycentricCoords.z;

    vec3 local_position = alpha * A.position + beta * B.position + gamma * C.position;
    vec3 local_normal = normalize(alpha * A.normal + beta * B.normal + gamma * C.normal);
    vertex.uv = alpha * A.uv + beta * B.uv + gamma * C.uv;

    vertex.P = vec3(gl_ObjectToWorldEXT * vec4(local_position, 1.0)); // transform position to world space

    vec3 geometric_normal = normalize(vec3(local_normal * gl_WorldToObjectEXT)); // transform normal to world space
    vertex.N = geometric_normal;
    vertex.geom_N = geometric_normal;

    vec3 tangent = normalize(alpha * A.tangent + beta * B.tangent + gamma * C.tangent);
    vertex.T = normalize(vec3(tangent * gl_WorldToObjectEXT)); // transform tangent to world space

    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    if (options.normal_mapping) {
        Material material = getMaterial(triangle.material_idx);
        vec3 texNormal = texture(material_textures[material.normal_tex_idx], vertex.uv).xyz;
        texNormal = texNormal * 2.0 - 1.0;

        mat3 TBN = getTBN(vertex.geom_N, vertex.T);
        vertex.N = normalize(TBN * texNormal);
    }

    return vertex;
}

PathVertex createVolumeBorderVertex(bool entering) {
    PathVertex vertex = createNewPathVertex();
    vertex.type = VOLUME_BOUNDARY_TYPE;

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);
    Vertex A = triangle.A;
    Vertex B = triangle.B;
    Vertex C = triangle.C;

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    float alpha = barycentricCoords.x;
    float beta = barycentricCoords.y;
    float gamma = barycentricCoords.z;

    vec3 local_position = alpha * A.position + beta * B.position + gamma * C.position;
    vertex.P = vec3(gl_ObjectToWorldEXT * vec4(local_position, 1.0));
    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    vertex.volume_idx = entering ? getVolumeIdx(triangle) : -1;

    return vertex;
}

PathVertex createVolumeVertex(vec3 pos, int volume_idx) {
    PathVertex vertex = createNewPathVertex();
    vertex.type = VOLUME_TYPE;

    vertex.P = pos;
    vertex.V = -normalize(gl_WorldRayDirectionEXT);

    vertex.volume_idx = volume_idx;
    vertex.local_volume_pos = (gl_WorldToObjectEXT * vec4(pos, 1.0f)).xyz;

    return vertex;
}

SampledSegment sampleNextSegment(PathVertex vertex, bool sample_bsdf, out bool specular_bounce, inout uvec4 rng_state) {
    SampledSegment sampled_segment = createNewSegment();

    mat3 TBN = getTBN(vertex.geom_N, vertex.T);
    mat3 transpose_tbn = transpose(TBN);

    Material material = getMaterial(vertex.material_idx);
    vec3 albedo = texture(material_textures[material.albedo_tex_idx], vertex.uv).xyz + material.albedo;
    vec3 metal_rough_ao = texture(material_textures[material.metal_rough_ao_tex_idx], vertex.uv).xyz;
    float metallic = metal_rough_ao.x + material.metallic;
    float roughness = metal_rough_ao.y + material.roughness;
    float ao = metal_rough_ao.z + material.ao;
    float eta = material.eta;

    if (sample_bsdf) {
        vec3 wo = normalize(transpose_tbn * vertex.V);

        BsdfSample brdf_sample = sampleBsfd(wo, albedo, metallic, roughness, eta, rng_state);

        vec3 sampled_dir = TBN * brdf_sample.wi;
        payload.next_dir = sampled_dir;
        sampled_segment.bsdf *= brdf_sample.f * abs(dot(sampled_dir, vertex.N));
        sampled_segment.dir_pdf *= brdf_sample.pdf;
        if (isTransmissive(brdf_sample.flags))
            payload.eta_scale *= sqr(brdf_sample.eta);
    } else {
        //payload.next_direction = TBN * sampleCosHemisphere(payload.rng_state);
        vec3 sampled_dir = sampleUniformSphere(rng_state);
        payload.next_dir = sampled_dir;

        vec3 wo = normalize(transpose_tbn * vertex.V);
        vec3 wi = normalize(transpose_tbn * sampled_dir);
        //payload.beta *= calcBRDF(wo, wi, albedo, metallic, roughness) * PI;
        sampled_segment.bsdf *= computeBsdf(wo, wi, albedo, metallic, roughness, eta) * abs(dot(sampled_dir, vertex.N));
        sampled_segment.dir_pdf *= INV_4_PI;
    }

    specular_bounce = effectivelySmooth(roughness, roughness);

    return sampled_segment;
}

SampledSegment sampleVolumeSegment(vec3 dir, EvaluatedVolume volume, vec3 dist_pdf, vec3 delta_tracking_pdf, inout uvec4 rng_state) {
    SampledSegment segment = createNewSegment();
    segment.dist_pdf *= dist_pdf;
    segment.delta_pdf *= delta_tracking_pdf;

    if (payload.sampling_options.similarity_relation) {
        if (options.sample_bsdf) {
            PhaseFunctionSample alt_sample = sampleAlteredPhaseFunctionIdx(-dir, volume.similarity_idx, rng_state);
            payload.next_dir = alt_sample.wi;
            segment.bsdf *= volume.scattering * alt_sample.p;
            segment.dir_pdf *= alt_sample.pdf;
        } else {
            PhaseFunctionSample iso_sample = sampleIsoPhaseFunction(-dir, rng_state);
            payload.next_dir = iso_sample.wi;
            float phase = evaluateAlteredPhaseFunctionIdx(-dir, iso_sample.wi, volume.similarity_idx);
            segment.bsdf *= volume.scattering * phase;
            segment.dir_pdf *= iso_sample.pdf;
        }
        return segment;
    }

    if (options.sample_bsdf) {
        PhaseFunctionSample hg_sample = sampleHGPhaseFunction(-dir, volume.g, rng_state);
        payload.next_dir = hg_sample.wi;
        segment.bsdf *= volume.scattering * hg_sample.p;
        segment.dir_pdf *= hg_sample.pdf;
    } else {
        PhaseFunctionSample iso_sample = sampleIsoPhaseFunction(-dir, rng_state);
        payload.next_dir = iso_sample.wi;
        float phase = henyeyGreenstein(-dir, iso_sample.wi, volume.g);
        segment.bsdf *= volume.scattering * phase;
        segment.dir_pdf *= iso_sample.pdf;
    }


    // This is the full version, without terms cut out for the specific phase function used
    //payload.beta *= scattering * transmittance(traveled_distance, extinction) * p_sample.p / distanceSamplingPdf(traveled_distance, extinction) / p_sample.pdf;
    // This is the version working for homogenous volumes (transmittance and distancePDF can also be cut)
    //payload.beta *= scattering * transmittance(traveled_distance, volume.majorant) / distanceSamplingPdf(traveled_distance, volume.majorant);

    return segment;
}

vec2 sampleChannel(EvaluatedVolume volume, inout uvec4 rng_state) {
    int lambda = min(int(stepAndOutputRNGFloat(rng_state) * 3.0), 2);

    float sampled_scattering;
    float sampled_absorption;

    switch (lambda)
    {
        case 0:
            sampled_scattering = volume.scattering.x;
            sampled_absorption = volume.absorption.x;
            break;

        case 1:
            sampled_scattering = volume.scattering.y;
            sampled_absorption = volume.absorption.y;
            break;

        default:
            sampled_scattering = volume.scattering.z;
            sampled_absorption = volume.absorption.z;
            break;
    }

    return vec2(sampled_scattering, sampled_absorption);
}

PathVertex deltaTracking(vec3 origin, vec3 dir, int volume_idx, inout uvec4 rng_state) {
    float dist_to_boundary = gl_HitTEXT;
    float tracked_dist = 0;

    VolumeInstance volume_instance = getVolume(volume_idx);
    vec3 obj_pos = (gl_WorldToObjectEXT * vec4(origin, 1.0f)).xyz;
    EvaluatedVolume volume = evaluateVolumeAtLocalPos(volume_instance, payload.sampling_options.similarity_relation, obj_pos);

    vec3 delta_tracking_pdf = vec3(1);
    vec3 accumulated_transmittance = vec3(1);
    vec3 accumulated_null_scattering = vec3(1);
    vec3 dist_pdf = vec3(1);

    while (true) {
        float sampled_dist = sampleDistance(volume.majorant, rng_state);
        float transmittance = transmittance(min(sampled_dist, dist_to_boundary - tracked_dist), volume.majorant);
        tracked_dist += sampled_dist;

        accumulated_transmittance *= transmittance;
        if (tracked_dist >= dist_to_boundary) {
            dist_pdf *= transmittance;
            break;
        } else {
            dist_pdf *= transmittance;
        }

        dist_pdf *= volume.majorant;
        delta_tracking_pdf /= volume.majorant;

        vec3 curr_pos = origin + tracked_dist * dir;

        obj_pos = (gl_WorldToObjectEXT * vec4(curr_pos, 1.0f)).xyz;
        volume = evaluateVolumeAtLocalPos(volume_instance, payload.sampling_options.similarity_relation, obj_pos);

        vec2 sampled_channel = sampleChannel(volume, rng_state);
        float sampled_scattering = sampled_channel.x;
        float sampled_absorption = sampled_channel.y;
        float sampled_extinction = sampled_scattering + sampled_absorption;
        vec3 null_scattering = vec3(volume.majorant) - (volume.scattering + volume.absorption);

        float p_real = sampled_extinction / volume.majorant;
        float rand = stepAndOutputRNGFloat(rng_state);

        if (rand < p_real) { // real collision
             PathVertex vertex = createVolumeVertex(curr_pos, volume_idx);

             delta_tracking_pdf *= sampled_extinction;
             SampledSegment segment = sampleVolumeSegment(dir, volume, dist_pdf, delta_tracking_pdf, rng_state);
             segment.transmittance *= accumulated_transmittance;
             segment.null_scattering *= accumulated_null_scattering;
             payload.next_segment = segment;
             return vertex;
        } else { // null collision
             accumulated_null_scattering *= null_scattering;
             delta_tracking_pdf *= (volume.majorant - sampled_extinction);
             continue;
        }
    }

    if (tracked_dist >= dist_to_boundary) {
        // exit volume
        SampledSegment segment = createNewSegment();
        segment.delta_pdf *= delta_tracking_pdf;
        segment.dist_pdf *= dist_pdf;
        segment.transmittance *= accumulated_transmittance;
        segment.null_scattering *= accumulated_null_scattering;
        payload.next_segment = segment;
        return createVolumeBorderVertex(false);
    }

    // should not be reachable
     return createVolumeBorderVertex(false);
}

PathVertex sampleVertexInHomogenous(vec3 origin, vec3 dir, int volume_idx, inout uvec4 rng_state) {
    float dist_to_boundary = gl_HitTEXT;

    VolumeInstance volume_instance = getVolume(volume_idx);

    EvaluatedVolume volume = evaluateHomoVolume(volume_instance, payload.sampling_options.similarity_relation);
    vec3 extinction = volume.scattering + volume.absorption;

    // hero wavelength sampling: pick one channel uniformly, sample distance from its exponential
    int hero = min(int(stepAndOutputRNGFloat(rng_state) * 3.0), 2);
    float hero_extinction = extinction[hero];
    float sampled_dist = sampleDistance(hero_extinction, rng_state);

    if (sampled_dist < dist_to_boundary) {
        PathVertex vertex = createVolumeVertex(origin + sampled_dist * dir, volume_idx);
        vec3 transmittance_vec = exp(-extinction * sampled_dist);
        // balance heuristic MIS across the three hero-channel strategies
        vec3 scatter_pdfs = extinction * transmittance_vec;
        vec3 dist_pdf = vec3((scatter_pdfs.x + scatter_pdfs.y + scatter_pdfs.z) / 3.0);
        SampledSegment segment = sampleVolumeSegment(dir, volume, dist_pdf, vec3(1), rng_state);
        segment.transmittance *= transmittance_vec;
        payload.next_segment = segment;
        return vertex;
    } else {
        SampledSegment segment = createNewSegment();
        vec3 transmittance_vec = exp(-extinction * dist_to_boundary);
        vec3 dist_pdf = vec3((transmittance_vec.x + transmittance_vec.y + transmittance_vec.z) / 3.0);
        segment.dist_pdf *= dist_pdf;
        segment.transmittance *= transmittance_vec;
        payload.next_segment = segment;
        return createVolumeBorderVertex(false);
    }

    // should not be reachable
     return createVolumeBorderVertex(false);
}

void main() {
    PathVertex vertex;
    vec3 last_P = payload.next_vertex.P;
    int last_volume_idx = getVertexVolumeIdx(payload.next_vertex);

    Triangle triangle = getTriangle(gl_InstanceCustomIndexEXT, gl_PrimitiveID);

    if (isVolumeBoundary(triangle)) {
        if (last_volume_idx >= 0) {
            // exiting volume or scattering inside
            if (payload.sampling_options.assume_homogenous) {
                vertex = sampleVertexInHomogenous(last_P, normalize(gl_WorldRayDirectionEXT), getVolumeIdx(triangle), payload.rng_state);
            } else {
                vertex = deltaTracking(last_P, normalize(gl_WorldRayDirectionEXT), getVolumeIdx(triangle), payload.rng_state);
            }
        } else {
            // entering volume
            vertex = createVolumeBorderVertex(true);
            payload.next_segment = createNewSegment();
        }
    } else {
        vertex = createSurfaceVertex();

        EvaluatedMaterial material = evaluateVertexMaterial(vertex);

        bool specular_bounce = false;
        payload.next_segment = sampleNextSegment(vertex, options.sample_bsdf, specular_bounce, payload.rng_state);
        vertex.is_specular = specular_bounce;
    }

    payload.next_vertex = packPathVertex(vertex);
}
