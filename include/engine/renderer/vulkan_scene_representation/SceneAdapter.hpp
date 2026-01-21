#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP

#include <GeometryManager.hpp>
#include <InstanceManager.hpp>
#include <OptionsWindow.hpp>
#include <VulkanContext.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "MaterialManager.hpp"

namespace RtEngine {
	class SceneAdapter {
	public:
		struct SceneInfo
		{
			std::string path;
			uint32_t emitting_instances_count;
		};

		SceneAdapter() = default;
		SceneAdapter(const std::shared_ptr<VulkanContext> &vulkanContext,
					 const std::shared_ptr<TextureRepository>& texture_repository,
					 const uint32_t max_frames_in_flight,
					 const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) :
			vulkan_context(vulkanContext), texture_repository(texture_repository),
			max_frames_in_flight(max_frames_in_flight) {

			instance_manager = std::make_shared<InstanceManager>(vulkan_context->resource_builder);
			geometry_manager = std::make_shared<GeometryManager>(vulkan_context);
			material_manager = std::make_shared<MaterialManager>(vulkan_context->resource_builder, texture_repository);

			main_deletion_queue.pushFunction([&]() {
				geometry_manager->destroy();
				instance_manager->destroy();
				material_manager->destroy();
			});

			createSceneLayout();
			createSceneDescriptorSets(scene_descriptor_set_layout);
			initDefaultResources(raytracingProperties);
		}

		void loadNewScene(const std::shared_ptr<IScene> &new_scene);

		void updateScene(const std::shared_ptr<DrawContext> &draw_context, uint32_t current_frame, uint32_t update_flags);
		void updateRenderTarget(std::shared_ptr<RenderTarget> target);

		void clearResources();

		std::shared_ptr<Material> getMaterial() const;
		VkDescriptorSet getSceneDescriptorSet(uint32_t frame_index) const;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<TextureRepository> texture_repository;

		std::unordered_map<std::string, std::shared_ptr<Material>> defaultMaterials;

	private:
		void createSceneLayout();
		void createSceneDescriptorSets(const VkDescriptorSetLayout &layout);
		void createUniformBuffers(const std::shared_ptr<IScene> &scene);

		void initDefaultResources(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties);
		void createDefaultSamplers();
		void createDefaultMaterials(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties);

		void setupNewScene(const std::shared_ptr<IScene> &scene);
		void createTlas();

		void updateGeometryResources(const std::shared_ptr<IScene> &scene);
		void updateStaticGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags);
		void updateDynamicGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags);

		void updateSceneDescriptorSets();
		void updateTlas(std::vector<RenderObject> objects) const;

		void updateMaterial(const std::shared_ptr<IScene> &scene) const;

		void updateSceneData(const std::shared_ptr<IScene> &scene, const std::shared_ptr<DrawContext> &draw_context, uint32_t current_frame) const;

		DeletionQueue main_deletion_queue, scene_resource_deletion_queue;
		uint32_t max_frames_in_flight;

		std::shared_ptr<IScene> loaded_scene;

		VkSampler defaultSamplerLinear;
		VkSampler defaultSamplerNearest;
		VkSampler defaultSamplerAnisotropic;

		std::shared_ptr<GeometryManager> geometry_manager;
		std::shared_ptr<InstanceManager> instance_manager;
		std::shared_ptr<MaterialManager> material_manager;

		std::shared_ptr<AccelerationStructure> top_level_acceleration_structure;

		VkDescriptorSetLayout scene_descriptor_set_layout;
		std::vector<VkDescriptorSet> scene_descriptor_sets{};
		std::vector<AllocatedBuffer> sceneUniformBuffers;
		std::vector<void *> sceneUniformBuffersMapped;
	};

} // namespace RtEngine
#endif // SCENEMANAGER_HPP
