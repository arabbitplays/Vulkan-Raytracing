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

    sceneData->ambientColor = glm::vec4(0.05f);

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
        glm::translate(glm::mat4(1.0f), glm::vec3{ 0,-1.0f,-5.5f }),
        proj
    );

    camera->image_width = image_width;
    camera->image_height = image_height;
}

void PlaneScene::initScene() {
    std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Sphere", "../ressources/models/sphere.obj"));
    meshes.push_back(meshAsset);

    meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Plane", "../ressources/models/plane.obj"));
    meshes.push_back(meshAsset);

    for (auto& meshAsset : meshes) {
        deletion_queue.pushFunction([&]() {
            mesh_builder->destroyMeshAsset(*meshAsset);
        });
    }
    float sphere_scale = 1.4f;

    std::shared_ptr<MeshNode> sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 0.7f, 0.0))
        * glm::scale( glm::mat4(1.0f), glm::vec3(sphere_scale));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec3(0.0f),
        1);
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["MirrorSphere"] = std::move(sphere);

    sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 0.7f, 0.0f))
        * glm::scale( glm::mat4(1.0f), glm::vec3(sphere_scale));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.9f),
        1,
        glm::vec3(1.f) / glm::vec3(1.03f, 1.04f, 1.05f));
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["GlassSphere"] = std::move(sphere);

    std::shared_ptr<MeshNode> quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)) * glm::scale( glm::mat4(1.0f), glm::vec3(8.f, 1.0f, 8.f));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = phong_material->createInstance(
        glm::vec3(0.2f),
        glm::vec3(0.0f),
        glm::vec3(0.02f),
        glm::vec3(0.4f),
        glm::vec3(0.0f),
        1.0f);
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Floor"] = std::move(quad);

    pointLights[0] = PointLight(glm::vec3(0, 1.5f, 3), glm::vec3(1, 0, 0), 10);
    sun = DirectionalLight(glm::vec3(-1,-1,-1), glm::vec3(1.0f), 1.0f);

    environment_map[0] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posx.jpg");
    environment_map[1] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negx.jpg");
    environment_map[2] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posy.jpg");
    environment_map[3] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negy.jpg");
    environment_map[4] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posz.jpg");
    environment_map[5] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negz.jpg");

    deletion_queue.pushFunction([&] () {
        for (auto image : environment_map) {
            ressource_builder.destroyImage(image);
        }
    });
}

void PlaneScene::update(uint32_t image_width, uint32_t image_height) {
    if (image_width != camera->image_width || image_height != camera->image_height) {
        initCamera(image_width, image_height);
    }
}

// -----------------------------------------------------------------------------------------------------------------------

void CornellBox::initCamera(uint32_t image_width, uint32_t image_height) {
    glm::mat4 proj = glm::perspective(glm::radians(65.0f),
        image_width / (float)image_height,
        0.1f, 512.0f);
    proj[1][1] *= -1; // flip y-axis because glm is for openGL
    /*camera = std::make_shared<Camera>(
        glm::lookAt(glm::vec3(0, 4, 8), glm::vec3(0, 2.5f, 0), glm::vec3(0, 1, 0)),
        proj
    );*/

    auto interactive_camera = std::make_shared<InteractiveCamera>(proj);
    interactive_camera->position = glm::vec3(0.0f, 5.0f, 0.0f);
    camera = interactive_camera;

    camera->image_width = image_width;
    camera->image_height = image_height;
}

void CornellBox::initScene() {
    std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Sphere", "../ressources/models/sphere.obj"));
    meshes.push_back(meshAsset);

    meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Plane", "../ressources/models/plane.obj"));
    meshes.push_back(meshAsset);

    for (auto& meshAsset : meshes) {
        deletion_queue.pushFunction([&]() {
            mesh_builder->destroyMeshAsset(*meshAsset);
        });
    }

    glm::vec3 diffuse_gray = glm::vec3(0.5f);
    float quad_scale = 5.0f;

    std::shared_ptr<MeshNode> quad = nullptr;
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            quad = std::make_shared<MeshNode>();
            quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f + i, j + 0.5f, -5.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))
                * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale / 10, 1.0f, quad_scale / 10));
            quad->worldTransform = glm::mat4{1.0f};
            quad->children = {};
            quad->meshAsset = meshes[1];
            if ((i % 2 == 0 && j % 2 == 0) || (i % 2 == 1 && j % 2 == 1)) {
                quad->meshMaterial = phong_material->createInstance(
                glm::vec3(0.0f, 0.0f, 0.5f),
                glm::vec3(0.4f),
                glm::vec3(0.0f, 0.0f, 0.02f),
                glm::vec3(0.0f),
                glm::vec3(0.0f),
                10.0f);
            } else {
                quad->meshMaterial = phong_material->createInstance(
                diffuse_gray,
                glm::vec3(0.4f),
                glm::vec3(0.0f),
                glm::vec3(0.0f),
                glm::vec3(0.0f),
                10.0f);
            }
            quad->refreshTransform(glm::mat4(1.0f));
            int index = i * 10 + j;
            nodes["Back" + std::to_string(index)] = std::move(quad);
        }
    }

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = phong_material->createInstance(
        glm::vec3(0.2f),
        glm::vec3(0.0f),
        glm::vec3(0.02f),
        glm::vec3(0.4f),
        glm::vec3(0.0f),
        1.0f);
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Floor"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = phong_material->createInstance(
        diffuse_gray,
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        1.0f);
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Ciel"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 5.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = phong_material->createInstance(
        glm::vec3(0.5f, 0.0f, 0.0f),
        glm::vec3(0.5f),
        glm::vec3(0.02f, 0.0f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        5.0f);
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Left"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f, 0.5f, 0.0f),
        glm::vec3(0.5f),
        glm::vec3(0.0f, 0.02f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        5.0f);
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Right"] = std::move(quad);

    float big_sphere_scale = 1.3f;

    std::shared_ptr<MeshNode> sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 6.5f, -2.5f))
        * glm::scale( glm::mat4(1.0f), glm::vec3(big_sphere_scale));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec3(0.0f),
        1);
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["MirrorSphere1"] = std::move(sphere);

    sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 2.0f, -2.5f))
        * glm::scale( glm::mat4(1.0f), glm::vec3(big_sphere_scale));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec3(0.0f),
        1);
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["MirrorSphere2"] = std::move(sphere);

    sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 2.0f, -2.5f))
        * glm::scale( glm::mat4(1.0f), glm::vec3(big_sphere_scale));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.9f),
        1,
        glm::vec3(1.f) / glm::vec3(1.03f, 1.06f, 1.09f));
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["GlassSphere1"] = std::move(sphere);

    sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 6.5f, -2.5f))
        * glm::scale( glm::mat4(1.0f), glm::vec3(big_sphere_scale));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = phong_material->createInstance(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.9f),
        1,
        glm::vec3(1.f) / glm::vec3(1.03f, 1.06f, 1.09f));
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["GlassSphere2"] = std::move(sphere);

    for (int i = 0; i < 5; i++) {
        sphere = std::make_shared<MeshNode>();
        sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f + 8.0f / 4.0f * i, 0.5f, 2.0f)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5f));
        sphere->worldTransform = glm::mat4{1.0f};
        sphere->children = {};
        sphere->meshAsset = meshes[0];
        sphere->meshMaterial = phong_material->createInstance(
            glm::vec3(0.0f, 0.5f, 0.5f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.0f, 0.2f, 0.2f),
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            std::pow(5.f, i));
        sphere->refreshTransform(glm::mat4(1.0f));
        nodes["Sphere" + std::to_string(i)] = std::move(sphere);
    }

    pointLights[0] = PointLight(glm::vec3(0, 8.0f, 3), glm::vec3(1, 0, 0), 20);
    sun = DirectionalLight(glm::vec3(-1,-1,-1), glm::vec3(1.0f), 1.0f);

    environment_map[0] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posx.jpg");
    environment_map[1] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negx.jpg");
    environment_map[2] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posy.jpg");
    environment_map[3] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negy.jpg");
    environment_map[4] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posz.jpg");
    environment_map[5] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negz.jpg");

    deletion_queue.pushFunction([&] () {
        for (auto image : environment_map) {
            ressource_builder.destroyImage(image);
        }
    });
}

void CornellBox::update(uint32_t image_width, uint32_t image_height) {
    if (image_width != camera->image_width || image_height != camera->image_height) {
        initCamera(image_width, image_height);
    }

    camera->update();
}

// -----------------------------------------------------------------------------------------------------------------------

void PBR_CornellBox::initCamera(uint32_t image_width, uint32_t image_height) {
    glm::mat4 proj = glm::perspective(glm::radians(65.0f),
        image_width / (float)image_height,
        0.1f, 512.0f);
    proj[1][1] *= -1; // flip y-axis because glm is for openGL
    camera = std::make_shared<Camera>(
        glm::lookAt(glm::vec3(0, 4, 10), glm::vec3(0, 3.5f, 0), glm::vec3(0, 1, 0)),
        proj
    );

    /*auto interactive_camera = std::make_shared<InteractiveCamera>(proj);
    interactive_camera->position = glm::vec3(0.0f, 5.0f, 0.0f);
    camera = interactive_camera;*/

    camera->image_width = image_width;
    camera->image_height = image_height;
}

void PBR_CornellBox::initScene() {
    std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Sphere", "../ressources/models/sphere.obj"));
    meshes.push_back(meshAsset);

    meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Plane", "../ressources/models/plane.obj"));
    meshes.push_back(meshAsset);

    for (auto& meshAsset : meshes) {
        deletion_queue.pushFunction([&]() {
            mesh_builder->destroyMeshAsset(*meshAsset);
        });
    }

    glm::vec3 diffuse_gray = glm::vec3(0.5f);
    float quad_scale = 5.0f;
    std::shared_ptr<MeshNode> quad = nullptr;
    std::shared_ptr<MaterialInstance> blue_instance = metal_rough->createInstance({.albedo = glm::vec3(0.0f, 0.0f, 0.5f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f});
    std::shared_ptr<MaterialInstance> grey_instance = metal_rough->createInstance({.albedo = diffuse_gray, .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f});

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            quad = std::make_shared<MeshNode>();
            quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f + i, j + 0.5f, -5.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))
                * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale / 10, 1.0f, quad_scale / 10));
            quad->worldTransform = glm::mat4{1.0f};
            quad->children = {};
            quad->meshAsset = meshes[1];
            if ((i % 2 == 0 && j % 2 == 0) || (i % 2 == 1 && j % 2 == 1)) {
                quad->meshMaterial = blue_instance;
            } else {
                quad->meshMaterial = grey_instance;
            }
            quad->refreshTransform(glm::mat4(1.0f));
            int index = i * 10 + j;
            nodes["Back" + std::to_string(index)] = std::move(quad);
        }
    }

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = metal_rough->createInstance({.albedo = glm::vec3(.6f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f});

    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Floor"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = metal_rough->createInstance({.albedo = diffuse_gray, .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f});
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Ciel"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 5.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = metal_rough->createInstance({.albedo = glm::vec3(0.5f, 0.0f, 0.0f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f});
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Left"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = metal_rough->createInstance({.albedo = glm::vec3(0.0f, 0.5f, 0.0f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f});
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Right"] = std::move(quad);

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            auto sphere = std::make_shared<MeshNode>();
            sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f + 6.0f / 4.0f * i, 1.0f + 6.0f / 4.0f * j, -2.0f)) * glm::scale(glm::mat4(1.0), glm::vec3(0.5f));
            sphere->worldTransform = glm::mat4{1.0f};
            sphere->children = {};
            sphere->meshAsset = meshes[0];
            sphere->meshMaterial = metal_rough->createInstance({
                .albedo = glm::vec3(0.5f, 0.0f, 0.0f),
                .metallic = std::clamp(0.2f * i, 0.1f, 0.99f),
                .roughness = std::clamp(0.2f * j, 0.1f, 0.99f),
                .ao = 1.0f});
            sphere->refreshTransform(glm::mat4(1.0f));
            nodes["Sphere" + std::to_string(i) + std::to_string(j)] = std::move(sphere);
        }
    }

    float light_scale = 1;
    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 9.8f, 0))
        * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(light_scale, 1.0f, light_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = metal_rough->createInstance({.albedo = glm::vec3(0.0f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f,
        .emission_color = glm::vec3(1.0f, 0.96f, 0.71f), .emission_power = 5});
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["AreaLight"] = std::move(quad);

    pointLights[0] = PointLight(glm::vec3(0, 8.0f, 3), glm::vec3(1, 1, 1), 20);
    //sun = DirectionalLight(glm::vec3(-1,-1,-1), glm::vec3(1.0f), 5.0f);

    environment_map[0] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posx.jpg");
    environment_map[1] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negx.jpg");
    environment_map[2] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posy.jpg");
    environment_map[3] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negy.jpg");
    environment_map[4] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posz.jpg");
    environment_map[5] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negz.jpg");

    deletion_queue.pushFunction([&] () {
        for (auto image : environment_map) {
            ressource_builder.destroyImage(image);
        }
    });
}

void PBR_CornellBox::update(uint32_t image_width, uint32_t image_height) {
    if (image_width != camera->image_width || image_height != camera->image_height) {
        initCamera(image_width, image_height);
    }

    camera->update();
}

// ---------------------------------------------------------------------------------------------------------------------

void Material_Showcase::initCamera(uint32_t image_width, uint32_t image_height) {
    glm::mat4 proj = glm::perspective(glm::radians(65.0f),
        image_width / (float)image_height,
        0.1f, 512.0f);
    proj[1][1] *= -1; // flip y-axis because glm is for openGL
    camera = std::make_shared<Camera>(
        glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -3.0f)),
     //glm::lookAt(glm::vec3(0, 4, 8), glm::vec3(0, 2.5f, 0), glm::vec3(0, 1, 0)),
        proj
    );

    /*auto interactive_camera = std::make_shared<InteractiveCamera>(proj);
    interactive_camera->position = glm::vec3(0.0f, 5.0f, 0.0f);
    camera = interactive_camera;*/

    camera->image_width = image_width;
    camera->image_height = image_height;
}

void Material_Showcase::initScene() {
    std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Sphere", "../ressources/models/sphere.obj"));
    meshes.push_back(meshAsset);

    meshAsset = std::make_shared<MeshAsset>(mesh_builder->LoadMeshAsset("Plane", "../ressources/models/plane.obj"));
    meshes.push_back(meshAsset);

    for (auto& meshAsset : meshes) {
        deletion_queue.pushFunction([&]() {
            mesh_builder->destroyMeshAsset(*meshAsset);
        });
    }

    glm::vec3 diffuse_gray = glm::vec3(0.5f);
    float quad_scale = 5.0f;

    deletion_queue.pushFunction([&]() {
        for (auto& texture : textures) {
            ressource_builder.destroyImage(texture);
        }
    });

    /*AllocatedImage albedo_tex = ressource_builder.loadTextureImage("../ressources/textures/testing/base-map.png");
    AllocatedImage metal_rough_ao_tex = ressource_builder.loadTextureImage("../ressources/textures/testing/base-map.png");
    AllocatedImage normal_tex = ressource_builder.loadTextureImage("../ressources/textures/testing/normal-map.png", VK_FORMAT_R8G8B8A8_UNORM);
    textures.push_back(albedo_tex);
    textures.push_back(metal_rough_ao_tex);
    textures.push_back(normal_tex);
    auto mat = metal_rough->createInstance(albedo_tex, metal_rough_ao_tex, normal_tex);

    auto quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 5, -5.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = mat;
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Back" ] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = mat;

    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Floor"] = std::move(quad);

    quad = std::make_shared<MeshNode>();
    quad->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 5.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))
        * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 0, 1))
        * glm::scale( glm::mat4(1.0f), glm::vec3(quad_scale, 1.0f, quad_scale));
    quad->worldTransform = glm::mat4{1.0f};
    quad->children = {};
    quad->meshAsset = meshes[1];
    quad->meshMaterial = mat;
    quad->refreshTransform(glm::mat4(1.0f));
    nodes["Left"] = std::move(quad);*/

    AllocatedImage rusty_albedo_tex = ressource_builder.loadTextureImage("../ressources/textures/rustyMetal/rusty-metal_albedo.png");
    AllocatedImage rusty_metal_rough_ao_tex = ressource_builder.loadTextureImage("../ressources/textures/rustyMetal/rusty-metal_metal_rough_ao.png");
    AllocatedImage rusty_normal_tex = ressource_builder.loadTextureImage("../ressources/textures/rustyMetal/rusty-metal_normal-dx.png", VK_FORMAT_R8G8B8A8_UNORM);

    textures.push_back(rusty_albedo_tex);
    textures.push_back(rusty_metal_rough_ao_tex);
    textures.push_back(rusty_normal_tex);

    auto sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0f));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = metal_rough->createInstance({.albedo_tex = rusty_albedo_tex, .metal_rough_ao_tex = rusty_metal_rough_ao_tex, .normal_tex = rusty_normal_tex});
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["Sphere1" ] = std::move(sphere);

    float light_size = 1;

    sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(2, 1, 3)) * glm::scale(glm::mat4(1.0), glm::vec3(light_size));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = metal_rough->createInstance({.albedo = glm::vec3(0.0f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f,
        .emission_color = glm::vec3(1), .emission_power = 10});
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["Light1" ] = std::move(sphere);

    sphere = std::make_shared<MeshNode>();
    sphere->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-2, 0.5f, 3)) * glm::scale(glm::mat4(1.0), glm::vec3(light_size));
    sphere->worldTransform = glm::mat4{1.0f};
    sphere->children = {};
    sphere->meshAsset = meshes[0];
    sphere->meshMaterial = metal_rough->createInstance({.albedo = glm::vec3(0.0f), .metallic = 0.5f, .roughness = 0.5f, .ao = 1.0f,
        .emission_color = glm::vec3(1), .emission_power = 10});
    sphere->refreshTransform(glm::mat4(1.0f));
    nodes["Light2" ] = std::move(sphere);

    //pointLights[0] = PointLight(glm::vec3(2, 2.0f, 2), glm::vec3(1, 1, 1), 20);
    //pointLights[1] = PointLight(glm::vec3(-2, 0.5f, 3), glm::vec3(1, 1, 1), 10);
    //sun = DirectionalLight(glm::vec3(-1,-1,-1), glm::vec3(1.0f), 10.0f);

    environment_map[0] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posx.jpg");
    environment_map[1] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negx.jpg");
    environment_map[2] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posy.jpg");
    environment_map[3] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negy.jpg");
    environment_map[4] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posz.jpg");
    environment_map[5] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negz.jpg");

    deletion_queue.pushFunction([&] () {
        for (auto image : environment_map) {
            ressource_builder.destroyImage(image);
        }
    });
}

void Material_Showcase::update(uint32_t image_width, uint32_t image_height) {
    if (image_width != camera->image_width || image_height != camera->image_height) {
        initCamera(image_width, image_height);
    }

    camera->update();

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    glm::mat4 rotation = glm::rotate(glm::mat4{1.0f}, time * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    //nodes["Sphere1"]->refreshTransform(rotation);
}


