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
    enum SceneBufferUpdateFlags
    {
        NO_UPDATE = 0,
        MATERIAL_UPDATE = 1 << 0,
        GEOMETRY_UPDATE = 1 << 1,
    };

    SceneManager() = default;
    SceneManager(std::shared_ptr<VulkanContext>& vulkanContext, uint32_t max_frames_in_flight, VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) : context(vulkanContext), max_frames_in_flight(max_frames_in_flight) {
        createSceneLayout();
        initDefaultResources(raytracingProperties);
    }

    void createScene(std::string scene_path);
    void createBlas();
    void updateScene(DrawContext& draw_context, uint32_t current_image_idx, AllocatedImage current_image, AllocatedImage& rng_tex);

    void clearRessources();

    std::shared_ptr<Material> getMaterial();
    uint32_t getEmittingInstancesCount();

    std::shared_ptr<VulkanContext> context;
    std::shared_ptr<Scene> scene;
    uint32_t bufferUpdateFlags = 0;
    std::string curr_scene_name;

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
    AllocatedBuffer createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createInstanceMappingBuffer(std::vector<RenderObject> &objects);
    AllocatedBuffer createEmittingInstancesBuffer(std::vector<RenderObject> &objects, std::shared_ptr<Material> material);
    AllocatedBuffer getEmittingInstancesBuffer();
    void createUniformBuffers();

    DeletionQueue main_deletion_queue, scene_resource_deletion_queue;
    uint32_t max_frames_in_flight;

    VkDescriptorSetLayout scene_descsriptor_set_layout;

    AllocatedImage whiteImage;
    AllocatedImage greyImage;
    AllocatedImage blackImage;
    AllocatedImage errorCheckerboardImage;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;
    VkSampler defaultSamplerAnisotropic;

    std::unordered_map<std::string, std::shared_ptr<Material>> defaultMaterials;
    std::shared_ptr<MetalRoughMaterial> metal_rough_material;
    std::shared_ptr<PhongMaterial> phong_material;

    AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer, instance_mapping_buffer, emitting_instances_buffer;
    std::shared_ptr<AccelerationStructure> top_level_acceleration_structure;

    uint32_t emitting_instances_count;
};



#endif //SCENEMANAGER_HPP
