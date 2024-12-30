//
// Created by oschdi on 12/30/24.
//

#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP

#include <OptionsWindow.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <RessourceBuilder.hpp>
#include <Scene.hpp>
#include <VulkanContext.hpp>

class SceneManager {
public:
    SceneManager() = default;
    SceneManager(std::shared_ptr<VulkanContext>& vulkanContext, uint32_t max_frames_in_flight) : context(vulkanContext), max_frames_in_flight(max_frames_in_flight) {
        createSceneLayout();
        initDefaultResources();
    }

    void createScene(SceneType scene_type);
    void clearRessources();

    PhongMaterial getMaterial(); // TODO make this material return type

    void createBlas();

    std::shared_ptr<VulkanContext> context;
    DeletionQueue main_deletion_queue, scene_ressource_deletion_queue;
    uint32_t max_frames_in_flight;

    std::shared_ptr<Scene> scene;

    VkDescriptorSetLayout scene_descsriptor_set_layout;
    std::vector<VkDescriptorSet> scene_descriptor_sets{};
    std::vector<AllocatedBuffer> sceneUniformBuffers;
    std::vector<void*> sceneUniformBuffersMapped;

    std::shared_ptr<PhongMaterial> phong_material;
    AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer;

private:
    void createSceneLayout();
    void initDefaultResources();

    void createDefaultTextures();
    void createDefaultSamplers();
    void createDefaultMaterials();

    void createSceneDescriptorSets();
    void createSceneBuffers();
    void createUniformBuffers();

    AllocatedImage whiteImage;
    AllocatedImage greyImage;
    AllocatedImage blackImage;
    AllocatedImage errorCheckerboardImage;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;
};



#endif //SCENEMANAGER_HPP
