//
// Created by oschdi on 2/3/25.
//

#ifndef RENDEREROPTIONS_H
#define RENDEREROPTIONS_H

enum SceneType {
    PBR_CORNELL_BOX,
    CORNELL_BOX,
    PLANE,
    SHOWCASE,
};

struct RendererOptions {
    SceneType scene_type = PBR_CORNELL_BOX;
    std::string output_path = "";
    int32_t sample_count = 1;
};

#endif //RENDEREROPTIONS_H
