#include "../denoising/svgf_layout.glsl"

void writeHistoryData(SvgfData data, inout SvgfHistData hist_data) {
    hist_data.color = data.color;
    hist_data.normal = data.normal;
    hist_data.depth = data.depth;
    //hist_data.instance_id = data.instance_id;
}

bool isSameSurface(SvgfData data, SvgfHistData hist_data) {
    return dot(data.normal, hist_data.normal) > 0.9
        && abs(data.depth - hist_data.depth) < max(0.001, 0.01 * data.depth);
        //&& data.instance_id == hist_data.instance_id;
}

void calculateMotionVector(inout SvgfData data) {
    vec4 prevClip = sceneData.last_view_proj * vec4(data.position, 1.0);
    if (prevClip.w <= 0.0) {
        data.motion = vec2(10000, 10000);
    }
    vec3 prevNDC = prevClip.xyz / prevClip.w;
    vec2 prev_uv = prevNDC.xy * 0.5 + 0.5;
    vec2 prev_screen_coord = prev_uv * vec2(gl_LaunchSizeEXT.xy);

    data.motion = prev_screen_coord - vec2(gl_LaunchIDEXT.xy);
}

void temporalFiltering(vec3 color, inout SvgfData data) {
    vec2 prev_screen_coord = vec2(gl_LaunchIDEXT.xy) + data.motion;

    ivec2 nearest_neighbor_screen_coord = ivec2(floor(prev_screen_coord + 0.5));
    nearest_neighbor_screen_coord = clamp(nearest_neighbor_screen_coord, ivec2(0), ivec2(gl_LaunchSizeEXT.xy) - ivec2(1, 1));

    uint prev_idx = nearest_neighbor_screen_coord.y * gl_LaunchSizeEXT.x + nearest_neighbor_screen_coord.x;
    SvgfHistData hist_data = getSvgfHistData(prev_idx);

    if (length(data.motion) < 100 && isSameSurface(data, hist_data)) {
        data.color = mix(data.color, color, 0.1);
    } else {
        data.color = color;
        //data.color = vec3(1, 0, 0);
    }
}