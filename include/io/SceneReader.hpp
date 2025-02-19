//
// Created by oschdi on 2/13/25.
//

#ifndef SCENEREADER_H
#define SCENEREADER_H

#include <Scene.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

class SceneReader {
public:
    SceneReader() = default;
    SceneReader(std::shared_ptr<VulkanContext>& vulkanContext)
        : context(vulkanContext) {}

    std::shared_ptr<Scene> readScene(const std::string& filename, std::unordered_map<std::string, std::shared_ptr<Material>> materials);
    std::shared_ptr<Camera> loadCamera(const YAML::Node& camera_node) const;
    void loadSceneLights(const YAML::Node& lights_node, std::shared_ptr<Scene>& scene);
    void initializeMaterial(const YAML::Node& material_node, std::shared_ptr<Material>& material, std::unordered_map<std::string, std::shared_ptr<Texture>> textures);

private:
    std::shared_ptr<Node> processSceneNodesRecursiv(const YAML::Node& yaml_node, const std::shared_ptr<Scene>& scene, const std::vector<std::shared_ptr<MaterialInstance>>& instances);

    std::shared_ptr<VulkanContext> context;
};



#endif //SCENEREADER_H
