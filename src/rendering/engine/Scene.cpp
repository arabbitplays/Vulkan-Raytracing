#include "Scene.hpp"

#include <PhongMaterial.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace RtEngine {
std::shared_ptr<SceneData> Scene::createSceneData() {
    auto sceneData = std::make_shared<SceneData>();

    sceneData->inverse_view = camera->getInverseView();
    sceneData->inverse_proj = camera->getInverseProjection();
    sceneData->view_pos = glm::vec4(camera->getPosition(), 0.0f);

    std::array<glm::vec4, POINT_LIGHT_COUNT> point_light_positions = {};
    std::array<glm::vec4, POINT_LIGHT_COUNT> point_light_colors = {};
    for (uint32_t i = 0; i < POINT_LIGHT_COUNT; i++) {
        point_light_positions[i] = glm::vec4{pointLights[i].position, pointLights[i].intensity};
        point_light_colors[i] = glm::vec4{pointLights[i].color, 0.0f};
    }

    sceneData->pointLightPositions = point_light_positions;
    sceneData->pointLightColors = point_light_colors;
    sceneData->sunlightDirection = glm::vec4(sun.direction,sun.intensity);
    sceneData->sunlightColor = glm::vec4(sun.color, 0.0f);
    sceneData->sunlightColor = glm::vec4{1, 0, 0, 1.0f};

    sceneData->ambientColor = glm::vec4(0.05f);

    return sceneData;
}

void Scene::addNode(std::string name, std::shared_ptr<Node> node)
{
    assert(!nodes.contains(name));
    nodes[name] = std::move(node);
}

std::shared_ptr<Node> Scene::getRootNode()
{
    return nodes["root"];
}


void Scene::update(uint32_t image_width, uint32_t image_height) {
    camera->update(image_width, image_height);

    for (auto& node : nodes)
    {
        node.second->update();
    }
}

void Scene::clearResources() {
    deletion_queue.flush();
}
}