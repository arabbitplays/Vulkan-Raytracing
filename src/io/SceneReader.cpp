//
// Created by oschdi on 2/13/25.
//

#include "SceneReader.hpp"

#include <MeshNode.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>
#include <YAML_glm.hpp>

std::shared_ptr<Scene> SceneReader::readScene(const std::string& filename, std::shared_ptr<Material> material)
{
    try {
        // Load YAML file
        YAML::Node config = YAML::LoadFile(filename);
        std::shared_ptr<Scene> scene = std::make_shared<Scene>(context->mesh_builder, *context->resource_builder, material);

        // Read values
        YAML::Node scene_node = config["scene"];

        YAML::Node camera_node = scene_node["camera"];
        scene->camera = std::make_shared<Camera>(
            context->swapchain->extent.width, context->swapchain->extent.height,
            camera_node["fov"].as<float>(),
            camera_node["position"].as<glm::vec3>(),
            camera_node["view_dir"].as<glm::vec3>()
        );

        for (const auto& mesh_node : scene_node["meshes"]) {
            scene->addMesh(mesh_node["name"].as<std::string>(), mesh_node["path"].as<std::string>());
        }

        for (const auto& yaml_mesh_node : scene_node["nodes"])
        {
            processSceneNodesRecursiv(static_cast<YAML::Node>(yaml_mesh_node), scene, material);
        }

        return scene;
    } catch (const YAML::Exception& e) {
        spdlog::error("YAML Error: {}", e.what());
        return nullptr;
    }
}

glm::mat4 recomposeMatrix(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale) {
    // Step 1: Create translation matrix
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);

    // Step 2: Convert Euler angles to quaternion, then to rotation matrix
    glm::quat quaternion = glm::quat(glm::radians(rotation)); // Convert degrees to radians
    glm::mat4 rotationMatrix = glm::toMat4(quaternion);

    // Step 3: Create scale matrix
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    // Step 4: Combine matrices (T * R * S)
    return translationMatrix * rotationMatrix * scaleMatrix;
}

void SceneReader::processSceneNodesRecursiv(const YAML::Node& yaml_node, const std::shared_ptr<Scene>& scene, const std::shared_ptr<Material>& material)
{
    // todo support non mesh nodes
    std::shared_ptr<MeshNode> scene_graph_node = std::make_shared<MeshNode>();
    scene_graph_node->localTransform = recomposeMatrix(yaml_node["translation"].as<glm::vec3>(), yaml_node["rotation"].as<glm::vec3>(), yaml_node["scale"].as<glm::vec3>());
    scene_graph_node->worldTransform = glm::mat4{1.0f};
    // todo process children recursivly
    scene_graph_node->children = {};
    scene_graph_node->meshAsset = scene->getMesh(yaml_node["mesh"].as<std::string>());
    if (typeid(*material) == typeid(MetalRoughMaterial) )
    {
        auto metal_rough_material = dynamic_cast<MetalRoughMaterial*>(material.get());

        MetalRoughParameters parameters{};

        YAML::Node material_node = yaml_node["material"];
        if (material_node["albedo"])
        {
            parameters.albedo = material_node["albedo"].as<glm::vec3>();
            parameters.metallic = material_node["metallic"].as<float>();
            parameters.roughness = material_node["roughness"].as<float>();
            parameters.ao = material_node["ao"].as<float>();
        }

        if (material_node["emission_power"])
        {
            parameters.emission_color = material_node["emission_color"].as<glm::vec3>();
            parameters.emission_power = material_node["emission_power"].as<float>();
        }

        scene_graph_node->meshMaterial = metal_rough_material->createInstance(parameters);

    }
    scene_graph_node->refreshTransform(glm::mat4(1.0f));
    scene->addNode(yaml_node["name"].as<std::string>(), scene_graph_node);
}
