#ifndef TEMPORAL_ANTIALIASING_GLSL
#define TEMPORAL_ANTIALIASING_GLSL

#include "../metalRough/g_buffer.glsl"

#define NORMAL_TEST_THRESHOLD 0.99
#define ABS_DEPTH_TEST_THRESHOLD 1e-4
#define REL_DEPTH_TEST_THRESHOLD 1e-6

#define COLOR_ACUMULATION_FACTOR 0.2

void writeHistoryData(GBufferData data, inout GBufferHistData hist_data) {
    hist_data.color = data.color;
    hist_data.normal = data.normal;
    hist_data.depth = data.depth;
    hist_data.instance_id = data.instance_id;
}

bool isSameSurface(GBufferData data, GBufferHistData hist_data) {
    return dot(data.normal, hist_data.normal) > NORMAL_TEST_THRESHOLD
    && abs(data.depth - hist_data.depth) < min(ABS_DEPTH_TEST_THRESHOLD, REL_DEPTH_TEST_THRESHOLD * data.depth)
    && data.instance_id == hist_data.instance_id;
}

vec3 getHistColorAt(GBufferData data, ivec2 screen_coord, inout bool is_valid) {
    ivec2 screen_size = imageSize(render_image);
    if (screen_coord.x < 0 || screen_coord.x >= screen_size.x || screen_coord.y < 0 || screen_coord.y >= screen_size.y) {
        is_valid = false;
        return vec3(0);
    }

    uint idx = screen_coord.y * screen_size.x + screen_coord.x;
    GBufferHistData hist_data = getGBufferHistData(idx);

    if (isSameSurface(data, hist_data)) {
        is_valid = true;
        return hist_data.color;
    } else {
        is_valid = false;
        return vec3(0);
    }
}

vec3 taaBilinear(GBufferData data, vec2 hist_screen_coord, inout bool is_valid) {
    ivec2 tab_screen_coord = ivec2(floor(hist_screen_coord));
    vec2 norm_coords = fract(hist_screen_coord);

    float weights[4] = float[4](
    (1 - norm_coords.x) * (1 - norm_coords.y),
    norm_coords.x * (1 - norm_coords.y),
    (1 - norm_coords.x) * norm_coords.y,
    norm_coords.x * norm_coords.y
    );
    float renormalization_weight = 0;

    vec3 samples[4] = vec3[4](vec3(0), vec3(0), vec3(0), vec3(0));

    samples[0] = getHistColorAt(data, tab_screen_coord, is_valid);
    if (is_valid) {
        renormalization_weight += weights[0];
    }

    samples[1] = getHistColorAt(data, tab_screen_coord + ivec2(1, 0), is_valid);
    if (is_valid) {
        renormalization_weight += weights[1];
    }

    samples[2] = getHistColorAt(data, tab_screen_coord + ivec2(0, 1), is_valid);
    if (is_valid) {
        renormalization_weight += weights[2];
    }

    samples[3] = getHistColorAt(data, tab_screen_coord + ivec2(1, 1), is_valid);
    if (is_valid) {
        renormalization_weight += weights[3];
    }

    if (renormalization_weight == 0) {
        is_valid = false;
        return vec3(0);
    }

    vec3 sum = vec3(0);
    for (uint i = 0; i < 4; i++) {
        sum += weights[i] / renormalization_weight * samples[i];
    }

    is_valid = true;
    return sum;
}

vec3 taaNearestNeighbor(GBufferData data, vec2 hist_screen_coord, inout bool is_valid) {
    ivec2 nearest_neighbor_screen_coord = ivec2(floor(hist_screen_coord + 0.5));
    return getHistColorAt(data, nearest_neighbor_screen_coord, is_valid);
}

void temporalFiltering(vec3 color, inout GBufferData data) {
    vec2 prev_screen_coord = vec2(gl_GlobalInvocationID.xy) + data.motion;
    bool valid_hist = false;
    vec3 hist_color = taaBilinear(data, prev_screen_coord, valid_hist);

    if (length(data.motion) < 100 && valid_hist) {
        data.color = mix(hist_color, color, COLOR_ACUMULATION_FACTOR);
    } else {
        data.color = color;
        //data.color = vec3(1, 0, 0);
    }
}

#endif // TEMPORAL_ANTIALIASING_GLSL