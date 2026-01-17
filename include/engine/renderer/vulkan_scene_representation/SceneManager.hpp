#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP

#include <GeometryManager.hpp>
#include <InstanceManager.hpp>
#include <OptionsWindow.hpp>
#include <VulkanContext.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "MaterialManager.hpp"
#include "../VulkanRenderer.hpp"

namespace RtEngine {
	class SceneManager {
	public:
		struct SceneInfo
		{
			std::string path;
			uint32_t emitting_instances_count;
		};

		enum SceneBufferUpdateFlags {
			NO_UPDATE = 0,
			MATERIAL_UPDATE = 1 << 0,
			GEOMETRY_UPDATE = 1 << 1,
		};

		SceneManager() = default;
		SceneManager(const std::shared_ptr<VulkanContext> &vulkanContext,
					 const std::shared_ptr<RuntimeContext>& runtime_context,
					 const uint32_t max_frames_in_flight,
					 const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) :
			vulkan_context(vulkanContext), runtime_context(runtime_context),
			max_frames_in_flight(max_frames_in_flight) {

			instance_manager = std::make_shared<InstanceManager>(vulkan_context->resource_builder);
			geometry_manager = std::make_shared<GeometryManager>(vulkan_context);
			material_manager = std::make_shared<MaterialManager>(vulkan_context->resource_builder, runtime_context->texture_repository);

			main_deletion_queue.pushFunction([&]() {
				geometry_manager->destroy();
				instance_manager->destroy();
				material_manager->destroy();
			});

			createUniformBuffers();
			createSceneLayout();
			createSceneDescriptorSets(scene_descriptor_set_layout);
			initDefaultResources(raytracingProperties);
		}

		void createScene(const std::string& scene_path);

		void setupNewScene(const std::shared_ptr<Scene>& scene);

		void createNewTlas();

		void updateGeometryResources(const std::shared_ptr<Scene> &scene);

		void updateStaticGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags);

		void updateDynamicGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags);

		void createBlas(std::vector<std::shared_ptr<MeshAsset>> &meshes);
		void updateScene(const std::shared_ptr<DrawContext> &draw_context);

		void clearResources();

		std::shared_ptr<Material> getMaterial() const;
		VkDescriptorSet getSceneDescriptorSet(uint32_t frame_index) const;
		SceneInfo getSceneInformation() const;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<RuntimeContext> runtime_context;
		std::shared_ptr<Scene> scene;
		uint32_t bufferUpdateFlags = 0;

	private:
		void createSceneLayout();
		void createSceneDescriptorSets(const VkDescriptorSetLayout &layout);
		void createUniformBuffers();

		void initDefaultResources(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties);
		void createDefaultSamplers();
		void createDefaultMaterials(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties);

		void updateSceneDescriptorSets(const std::shared_ptr<DrawContext> &draw_context);
		void updateTlas(std::vector<RenderObject> objects) const;

		void updateMaterial(const std::shared_ptr<Scene> &scene) const;

		void updateSceneData(const std::shared_ptr<DrawContext> &draw_context);

		DeletionQueue main_deletion_queue, scene_resource_deletion_queue;
		uint32_t max_frames_in_flight;

		VkSampler defaultSamplerLinear;
		VkSampler defaultSamplerNearest;
		VkSampler defaultSamplerAnisotropic;

		std::unordered_map<std::string, std::shared_ptr<Material>> defaultMaterials;

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
