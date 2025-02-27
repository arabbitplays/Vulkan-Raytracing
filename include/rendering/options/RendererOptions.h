//
// Created by oschdi on 2/3/25.
//

#ifndef RENDEREROPTIONS_H
#define RENDEREROPTIONS_H

#include <vector>

struct RendererOptions {
    std::string curr_scene_path = "";
    std::string resources_path = "";
    std::vector<std::string> scene_paths{};

    std::string reference_scene_path = ""; // used by the reference renderer to find the scene to render a erference of
    std::string reference_image_path = ""; // used by the benchmark renderer to find the image to benchmark against
    std::string output_dir = "";
    int32_t sample_count = 1;
};

#endif //RENDEREROPTIONS_H
