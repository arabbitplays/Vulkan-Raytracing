//
// Created by oschdi on 2/13/25.
//

#include "SceneReader.hpp"

#include <AccelerationStructure.hpp>
#include <Node.hpp>
#include <QuickTimer.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>
#include <YAML_glm.hpp>

std::shared_ptr<Scene> SceneReader::readScene(const std::string& filename, std::unordered_map<std::string, std::shared_ptr<Material>> materials)
{
    QuickTimer quick_timer("Reading scene from file");

    try {
        YAML::Node config = YAML::LoadFile(filename);
        YAML::Node scene_node = config["scene"];

        auto material_name = scene_node["material_name"].as<std::string>();
        if (!materials.contains(material_name))
            throw std::runtime_error("Material " + material_name + " does not exist");
        std::shared_ptr<Scene> scene = std::make_shared<Scene>(context->mesh_builder, *context->resource_builder, materials[material_name]);

        scene->camera = loadCamera(scene_node["camera"]);
        loadSceneLights(scene_node["lights"], scene);

        for (const auto& mesh_node : scene_node["meshes"]) {
            scene->addMesh(mesh_node["name"].as<std::string>(), mesh_node["path"].as<std::string>());
        }

        for (const auto& texture_node : scene_node["textures"]) {
            // todo handle more types
            TextureType type = texture_node["is_normal"].as<bool>() ? NORMAL : PARAMETER;
            scene->addTexture(texture_node["path"].as<std::string>(), type);
        }

        initializeMaterial(scene_node["materials"], materials[material_name], scene->textures);

        std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
        scene_graph_node->name = "root";
        glm::mat4 identity = glm::mat4(1.0f);
        scene_graph_node->transform->setLocalTransform(identity);
        scene_graph_node->children = {};
        for (const auto& yaml_mesh_node : scene_node["nodes"])
        {
            scene_graph_node->children.push_back(processSceneNodesRecursiv(static_cast<YAML::Node>(yaml_mesh_node), scene, scene->material->getInstances()));
        }
        scene->addNode(scene_graph_node->name, scene_graph_node);

        return scene;
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(e.what());
    }
}

std::shared_ptr<Camera> SceneReader::loadCamera(const YAML::Node& camera_node) const
{
    if (camera_node["interactive"].as<bool>())
    {
        return std::make_shared<InteractiveCamera>(
            context->swapchain->extent.width, context->swapchain->extent.height,
            camera_node["fov"].as<float>(),
            camera_node["position"].as<glm::vec3>(),
            camera_node["view_dir"].as<glm::vec3>()
        );
    } else
    {
        return std::make_shared<Camera>(
            context->swapchain->extent.width, context->swapchain->extent.height,
            camera_node["fov"].as<float>(),
            camera_node["position"].as<glm::vec3>(),
            camera_node["view_dir"].as<glm::vec3>()
        );
    }
}

void SceneReader::loadSceneLights(const YAML::Node& lights_node, std::shared_ptr<Scene>& scene)
{
    if (lights_node["sun"])
    {
        YAML::Node sun_node = lights_node["sun"];
        scene->sun = DirectionalLight(
            sun_node["direction"].as<glm::vec3>(),
            sun_node["color"].as<glm::vec3>(),
            sun_node["intensity"].as<float>());
    }

    uint32_t point_light_index = 0;
    for (auto& point_light_node : lights_node["point_lights"])
    {
        scene->pointLights[point_light_index++] = PointLight(
            point_light_node["position"].as<glm::vec3>(),
            point_light_node["color"].as<glm::vec3>(),
            point_light_node["intensity"].as<float>());
    }
}

void SceneReader::initializeMaterial(const YAML::Node& material_node, std::shared_ptr<Material>& material, std::unordered_map<std::string, std::shared_ptr<Texture>> textures)
{
    if (typeid(*material) == typeid(MetalRoughMaterial) )
    {
        auto metal_rough_material = dynamic_cast<MetalRoughMaterial*>(material.get());

        for (const auto& material_node : material_node)
        {
            MetalRoughParameters parameters{};

            if (material_node["albedo"])
            {
                parameters.albedo = material_node["albedo"].as<glm::vec3>();
                parameters.metallic = material_node["metallic"].as<float>();
                parameters.roughness = material_node["roughness"].as<float>();
                parameters.ao = material_node["ao"].as<float>();
                if (material_node["eta"])
                    parameters.eta = material_node["eta"].as<float>();
            } else
            {
                parameters.albedo_tex = textures[material_node["albedo_tex"].as<std::string>()];
                parameters.metal_rough_ao_tex = textures[material_node["metal_rough_ao_tex"].as<std::string>()];
                parameters.normal_tex = textures[material_node["normal_tex"].as<std::string>()];
            }

            if (material_node["emission_power"])
            {
                parameters.emission_color = material_node["emission_color"].as<glm::vec3>();
                parameters.emission_power = material_node["emission_power"].as<float>();
            }

            metal_rough_material->createInstance(parameters);
        }
    } else if (typeid(*material) == typeid(PhongMaterial)) {
        auto phong_material = dynamic_cast<PhongMaterial*>(material.get());

        for (const auto& material_node : material_node)
        {
            phong_material->createInstance(
                material_node["diffuse"].as<glm::vec3>(),
                material_node["specular"].as<glm::vec3>(),
                material_node["ambient"].as<glm::vec3>(),
                material_node["reflection"].as<glm::vec3>(),
                material_node["transmission"].as<glm::vec3>(),
                material_node["n"].as<float>(),
                material_node["eta"].as<glm::vec3>()
            );
        }
    }
}

glm::mat4 recomposeMatrix(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale) {
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);

    glm::quat quaternion = glm::quat(glm::radians(rotation)); // Convert degrees to radians
    glm::mat4 rotationMatrix = glm::toMat4(quaternion);

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

std::shared_ptr<Node> SceneReader::processSceneNodesRecursiv(const YAML::Node& yaml_node, const std::shared_ptr<Scene>& scene, const std::vector<std::shared_ptr<MaterialInstance>>& instances)
{
    if (yaml_node["mesh"]) // construct a mesh node
    {
        std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
        scene_graph_node->name = yaml_node["name"].as<std::string>();
        scene_graph_node->transform->setLocalTransform(recomposeMatrix(yaml_node["translation"].as<glm::vec3>(), yaml_node["rotation"].as<glm::vec3>(), yaml_node["scale"].as<glm::vec3>()));
        scene_graph_node->children = {};
        for (auto& child_node : yaml_node["children"])
        {
            scene_graph_node->children.push_back(processSceneNodesRecursiv(child_node, scene, instances));
        }
        //scene_graph_node->meshAsset = scene->getMesh(yaml_node["mesh"].as<std::string>());
        //scene_graph_node->meshMaterial = instances.at(yaml_node["material_idx"].as<int>());
        scene_graph_node->refreshTransform(glm::mat4(1.0f));

        scene->addNode(scene_graph_node->name, scene_graph_node);
        return scene_graph_node;
    } else // construct a plain node
    {
        std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
        scene_graph_node->name = yaml_node["name"].as<std::string>();
        scene_graph_node->transform->setLocalTransform(recomposeMatrix(yaml_node["translation"].as<glm::vec3>(), yaml_node["rotation"].as<glm::vec3>(), yaml_node["scale"].as<glm::vec3>()));
        scene_graph_node->children = {};
        for (auto& child_node : yaml_node["children"])
        {
            scene_graph_node->children.push_back(processSceneNodesRecursiv(child_node, scene, instances));
        }
        scene_graph_node->refreshTransform(glm::mat4(1.0f));

        scene->addNode(scene_graph_node->name, scene_graph_node);
        return scene_graph_node;
    }
}
