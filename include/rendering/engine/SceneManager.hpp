#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP

#include <GeometryManager.hpp>
#include <InstanceManager.hpp>
#include <MetalRoughMaterial.hpp>
#include <OptionsWindow.hpp>
#include <VulkanContext.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace RtEngine {
	class SceneManager {
	public:
		enum SceneBufferUpdateFlags {
			NO_UPDATE = 0,
			MATERIAL_UPDATE = 1 << 0,
			GEOMETRY_UPDATE = 1 << 1,
		};

		SceneManager() = default;
		SceneManager(std::shared_ptr<VulkanContext> &vulkanContext, std::shared_ptr<RuntimeContext> runtime_context,
					 uint32_t max_frames_in_flight,
					 VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) :
			vulkan_context(vulkanContext), runtime_context(runtime_context),
			max_frames_in_flight(max_frames_in_flight) {
			VkDevice device = vulkan_context->device_manager->getDevice();

			geometry_manager = std::make_shared<GeometryManager>(vulkan_context->resource_builder);
			instance_manager = std::make_shared<InstanceManager>(vulkan_context->resource_builder);
			top_level_acceleration_structure = std::make_shared<AccelerationStructure>(
					device, *vulkan_context->resource_builder, *vulkan_context->command_manager,
					VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

			main_deletion_queue.pushFunction([&]() {
				geometry_manager->destroy();
				instance_manager->destroy();
				top_level_acceleration_structure->destroy();
			});

			createSceneLayout();
			createSceneDescriptorSets(scene_descriptor_set_layout);
			initDefaultResources(raytracingProperties);
		}

		void createScene(std::string scene_path);
		void createBlas(std::vector<std::shared_ptr<MeshAsset>> &meshes);
		void updateScene(DrawContext &draw_context, uint32_t current_image_idx, const AllocatedImage &current_image,
						 const AllocatedImage &rng_tex);

		void clearResources();

		std::shared_ptr<Material> getMaterial() const;
		VkDescriptorSet getSceneDescriptorSet(uint32_t frame_index) const;
		uint32_t getEmittingInstancesCount();

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<RuntimeContext> runtime_context;
		std::shared_ptr<Scene> scene;
		uint32_t bufferUpdateFlags = 0;
		std::string curr_scene_name;

	private:
		void createSceneLayout();
		void createSceneDescriptorSets(const VkDescriptorSetLayout &layout);
		void createUniformBuffers();

		void initDefaultResources(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);
		void createDefaultTextures();
		void createDefaultSamplers();
		void createDefaultMaterials(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);

		void updateSceneDescriptorSets(uint32_t frame_idx, const AllocatedImage &current_image,
									   const AllocatedImage &rng_tex);
		void updateTlas(std::shared_ptr<AccelerationStructure> &tlas, std::vector<RenderObject> objects);

		DeletionQueue main_deletion_queue, scene_resource_deletion_queue;
		uint32_t max_frames_in_flight;

		AllocatedImage whiteImage;
		AllocatedImage greyImage;
		AllocatedImage blackImage;
		AllocatedImage errorCheckerboardImage;

		VkSampler defaultSamplerLinear;
		VkSampler defaultSamplerNearest;
		VkSampler defaultSamplerAnisotropic;

		std::unordered_map<std::string, std::shared_ptr<Material>> defaultMaterials;

		std::shared_ptr<GeometryManager> geometry_manager;
		std::shared_ptr<InstanceManager> instance_manager;

		std::shared_ptr<AccelerationStructure> top_level_acceleration_structure;

		VkDescriptorSetLayout scene_descriptor_set_layout;
		std::vector<VkDescriptorSet> scene_descriptor_sets{};
		std::vector<AllocatedBuffer> sceneUniformBuffers;
		std::vector<void *> sceneUniformBuffersMapped;
	};

} // namespace RtEngine
#endif // SCENEMANAGER_HPP
