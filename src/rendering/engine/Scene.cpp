//
// Created by oschdi on 12/18/24.
//

#include "Scene.hpp"

#include <MeshNode.hpp>
#include <PhongMaterial.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

std::shared_ptr<SceneData> Scene::createSceneData() {
    auto sceneData = std::make_shared<SceneData>();

    sceneData->inverse_view = camera->inverse_view;
    sceneData->inverse_proj = camera->inverse_projection;
    sceneData->view_pos = glm::vec4(camera->position, 0.0f);

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

    sceneData->ambientColor = glm::vec4(1.f);

    return sceneData;
}

void Scene::clearRessources() {
    deletion_queue.flush();
}


// -------------------------------------------------------------------------------------------------------------------------

void PlaneScene::initCamera(uint32_t image_width, uint32_t image_height) {
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
        image_width / (float)image_height,
        0.1f, 512.0f);
    proj[1][1] *= -1; // flip y-axis because glm is for openGL
    camera = std::make_shared<Camera>(
        glm::translate(glm::mat4(1.0f), glm::vec3{ 0,0,-4.5f }),
        proj
    );
}

void PlaneScene::initScene(std::shared_ptr<PhongMaterial> phong_material) {
    std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Sphere", "../ressources/models/sphere.obj"));
    meshes.push_back(meshAsset);

    meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Plane", "../ressources/models/plane.obj"));
    meshes.push_back(meshAsset);

    for (auto& meshAsset : meshes) {
        deletion_queue.pushFunction([&]() {
            mesh_builder->destroyMeshAsset(*meshAsset);
        });
    }

    for (int i = 0; i < 5; i++) {
        std::shared_ptr<MeshNode> sphere1 = std::make_shared<MeshNode>();
        sphere1->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-3.f + 6.0f / 4.0f * i, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
        sphere1->worldTransform = glm::mat4{1.0f};
        sphere1->children = {};
        sphere1->meshAsset = meshes[0];
        sphere1->meshMaterial = phong_material->createInstance(glm::vec3(0.0f, 0.5f, 0.5f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.0f, 0.2f, 0.2f), std::pow(5.f, i));
        sphere1->refreshTransform(glm::mat4(1.0f));
        nodes["Sphere" + i] = std::move(sphere1);
    }

    std::shared_ptr<MeshNode> plane = std::make_shared<MeshNode>();
    plane->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)) * glm::scale( glm::mat4(1.0f), glm::vec3(8.f, 1.0f, 8.f));
    plane->worldTransform = glm::mat4{1.0f};
    plane->children = {};
    plane->meshAsset = meshes[1];
    plane->meshMaterial = phong_material->createInstance(glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.5f), glm::vec3(0.02f, 0.0f, 0.0f), 5.0f);
    plane->refreshTransform(glm::mat4(1.0f));
    nodes["Plane"] = std::move(plane);

    pointLights[0] = PointLight(glm::vec3(0, 1.5f, 3), glm::vec3(1, 0, 0), 10);
    sun = DirectionalLight(glm::vec3(-1,-1,-1), glm::vec3(1.0f), 1.0f);
}

