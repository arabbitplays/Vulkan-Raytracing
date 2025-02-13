//
// Created by oschdi on 2/13/25.
//

#include "SceneReader.hpp"

#include <spdlog/spdlog.h>

std::shared_ptr<Scene> SceneReader::readScene(const std::string& filename, std::shared_ptr<Material> material)
{
    try {
        // Load YAML file
        YAML::Node config = YAML::LoadFile(filename);
        std::shared_ptr<Scene> scene = std::make_shared<Scene>(context->mesh_builder, *context->resource_builder, material);

        // Read values
        YAML::Node scene_node = config["scene"];
        return scene;
    } catch (const YAML::Exception& e) {
        spdlog::error("YAML Error: {}", e.what());
        return nullptr;
    }
}
