//
// Created by oschdi on 12/18/24.
//

#ifndef SCENE_HPP
#define SCENE_HPP

#include <array>
#include <Camera.hpp>
#include <InteractiveCamera.hpp>
#include <MeshAsset.hpp>
#include <MeshAssetBuilder.hpp>
#include <MetalRoughMaterial.hpp>
#include <Node.hpp>
#include <PhongMaterial.hpp>
#include <vector>
#include <glm/vec3.hpp>
#include <bits/shared_ptr.h>

#define POINT_LIGHT_COUNT 4

struct SceneData {
    glm::mat4 inverse_view;
    glm::mat4 inverse_proj;
    glm::vec4 view_pos;
    std::array<glm::vec4, POINT_LIGHT_COUNT> pointLightPositions;
    std::array<glm::vec4, POINT_LIGHT_COUNT> pointLightColors;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct PointLight {
    PointLight() = default;
    PointLight(const glm::vec3& position, const glm::vec3& color, float intensity) : position(position), color(color), intensity(intensity) {}

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(0.0f);
    float intensity = 0.0f;
};

struct DirectionalLight {
    DirectionalLight() = default;
    DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity) : direction(direction), color(color), intensity(intensity) {}

    glm::vec3 direction = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(0.0f);
    float intensity = 0.0f;
};

class Scene {
public:
    Scene(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder, RessourceBuilder& ressource_builder, std::shared_ptr<Material> material)
        : mesh_builder(mesh_asset_builder), ressource_builder(ressource_builder), material(material)
    {
        Scene::initScene();
    }

    virtual ~Scene() = default;

    void addMesh(std::string name, std::string path);
    std::shared_ptr<MeshAsset> getMesh(std::string name);
    void addTexture(std::string path, TextureType type);
    void addNode(std::string name, std::shared_ptr<Node> node);

    std::shared_ptr<SceneData> createSceneData();
    void update(uint32_t image_width, uint32_t image_height);
    std::vector<std::shared_ptr<MeshAsset>> getMeshes();
    void clearRessources();

    std::shared_ptr<Camera> camera;

    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::array<AllocatedImage, 6> environment_map{};

    DirectionalLight sun;
    std::array<PointLight, POINT_LIGHT_COUNT> pointLights{};

    std::shared_ptr<MeshAssetBuilder> mesh_builder;
    RessourceBuilder ressource_builder;
    DeletionQueue deletion_queue{};

    std::shared_ptr<Material> material;

protected:
    virtual void initCamera(uint32_t image_width, uint32_t image_height) {};
    virtual void initScene()
    {
        environment_map[0] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posx.jpg").image;
        environment_map[1] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negx.jpg").image;
        environment_map[2] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posy.jpg").image;
        environment_map[3] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negy.jpg").image;
        environment_map[4] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/posz.jpg").image;
        environment_map[5] = ressource_builder.loadTextureImage("../ressources/textures/environmentMaps/negz.jpg").image;

        deletion_queue.pushFunction([&] () {
            for (auto image : environment_map) {
                ressource_builder.destroyImage(image);
            }
        });

        pointLights[0] = PointLight(glm::vec3(0, 8.0f, 3), glm::vec3(1, 0, 0), 20);
        sun = DirectionalLight(glm::vec3(-1,-1,-1), glm::vec3(1.0f), 0.0f);
    };


};

class PlaneScene : public Scene {
public:
    PlaneScene() = default;
    PlaneScene(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder, RessourceBuilder& ressource_builder,
            uint32_t image_width, uint32_t image_height, std::shared_ptr<PhongMaterial> phong_material) : Scene(mesh_asset_builder, ressource_builder, phong_material), phong_material(phong_material) {
        initCamera(image_width, image_height);
        initScene();
    };

protected:
    void initCamera(uint32_t image_width, uint32_t image_height) override;
    void initScene() override;

    std::shared_ptr<PhongMaterial> phong_material;
};

class CornellBox : public Scene {
public:
    CornellBox() = default;
    CornellBox(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder, RessourceBuilder& ressource_builder
            , uint32_t image_width, uint32_t image_height, std::shared_ptr<PhongMaterial> phong_material) :
            Scene(mesh_asset_builder, ressource_builder, phong_material), phong_material(phong_material) {
        initCamera(image_width, image_height);
        initScene();
    };

protected:
    void initCamera(uint32_t image_width, uint32_t image_height) override;
    void initScene() override;

    std::shared_ptr<PhongMaterial> phong_material;
};

class PBR_CornellBox : public Scene {
public:
    PBR_CornellBox() = default;
    PBR_CornellBox(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder, RessourceBuilder& ressource_builder
            , uint32_t image_width, uint32_t image_height, std::shared_ptr<MetalRoughMaterial> metal_rough)
            : Scene(mesh_asset_builder, ressource_builder, metal_rough), metal_rough(metal_rough) {
        initCamera(image_width, image_height);
        initScene();
    };

protected:
    void initCamera(uint32_t image_width, uint32_t image_height) override;
    void initScene() override;

    std::shared_ptr<MetalRoughMaterial> metal_rough;
};

class Material_Showcase : public Scene {
public:
    Material_Showcase() = default;
    Material_Showcase(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder, RessourceBuilder& ressource_builder
            , uint32_t image_width, uint32_t image_height, std::shared_ptr<MetalRoughMaterial> metal_rough)
            : Scene(mesh_asset_builder, ressource_builder, metal_rough), metal_rough(metal_rough) {
        initCamera(image_width, image_height);
        initScene();
    };

protected:
    void initCamera(uint32_t image_width, uint32_t image_height) override;
    void initScene() override;

    std::shared_ptr<MetalRoughMaterial> metal_rough;
};

#endif //SCENE_HPP
