//
// Created by oschdi on 12/30/24.
//

#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP

#include <MetalRoughMaterial.hpp>
#include <OptionsWindow.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <RessourceBuilder.hpp>
#include <Scene.hpp>
#include <VulkanContext.hpp>

class SceneManager {
public:
    SceneManager() = default;
    SceneManager(std::shared_ptr<VulkanContext>& vulkanContext, uint32_t max_frames_in_flight, VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) : context(vulkanContext), max_frames_in_flight(max_frames_in_flight) {
        createSceneLayout();
        initDefaultResources(raytracingProperties);
    }

    void createScene(SceneType scene_type);
    void updateScene(DrawContext& draw_context, uint32_t current_image_idx, AllocatedImage& current_image);
    void clearRessources();

    std::shared_ptr<Material> getMaterial();

    void createBlas();

    std::shared_ptr<Scene> scene;
    SceneType curr_scene_type;

    std::vector<VkDescriptorSet> scene_descriptor_sets{};
    std::vector<AllocatedBuffer> sceneUniformBuffers;
    std::vector<void*> sceneUniformBuffersMapped;

private:
    void createSceneLayout();
    void initDefaultResources(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);

    void createDefaultTextures();
    void createDefaultSamplers();
    void createDefaultMaterials(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);

    void createSceneDescriptorSets();
    void createSceneBuffers();
    void createUniformBuffers();

    std::shared_ptr<VulkanContext> context;
    DeletionQueue main_deletion_queue, scene_ressource_deletion_queue;
    uint32_t max_frames_in_flight;

    VkDescriptorSetLayout scene_descsriptor_set_layout;

    AllocatedImage whiteImage;
    AllocatedImage greyImage;
    AllocatedImage blackImage;
    AllocatedImage errorCheckerboardImage;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;
    VkSampler defaultSamplerAnisotropic;

    std::shared_ptr<PhongMaterial> phong_material;
    std::shared_ptr<MetalRoughMaterial> metal_rough_material;

    AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer, instance_mapping_buffer;
    std::shared_ptr<AccelerationStructure> top_level_acceleration_structure;

};



#endif //SCENEMANAGER_HPP
