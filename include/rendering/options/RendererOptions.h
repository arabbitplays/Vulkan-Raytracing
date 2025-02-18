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

    std::string reference_scene_path = "";
    std::string output_dir = "";
    int32_t sample_count = 1;
};

#endif //RENDEREROPTIONS_H
