#include "../denoising/svgf_layout.glsl"

void writeHistoryData(SvgfData data, inout SvgfHistData hist_data) {
    hist_data.color = data.color;
    hist_data.normal = data.normal;
    hist_data.depth = data.depth;
}

bool isSameSurface(SvgfData data, inout SvgfHistData hist_data) {
    return dot(data.normal, hist_data.normal) - 1 < 0.001 && abs(data.depth - hist_data.depth) < 0.001;
}

void calculateMotionVector(SvgfData data) {

}

void temporalFiltering(vec3 color) {
    uint idx = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    SvgfData data = getSvgfData(idx);
    SvgfHistData hist_data = getSvgfHistData(idx);

    if (isSameSurface(data, hist_data)) {
        data.color = mix(data.color, color, 0.1);
    } else {
        data.color = color;
    }

    writeHistoryData(data, hist_data);
    setSvgfData(idx, data);
    setSvgfHistData(idx, hist_data);
}
