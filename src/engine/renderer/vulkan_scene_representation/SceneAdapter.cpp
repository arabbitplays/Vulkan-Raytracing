#include "SceneAdapter.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <MeshRenderer.hpp>
#include <MetalRoughMaterial.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>
#include <SceneUtil.hpp>

#include "PhongMaterial.hpp"
#include "UpdateFlags.hpp"

namespace RtEngine {

	void SceneAdapter::loadNewScene(const std::shared_ptr<IScene> &new_scene) {
		QuickTimer timer{"Scene Creation", true};

		scene_resource_deletion_queue.flush();

		loaded_scene = new_scene;

		setupNewScene(loaded_scene);
		updateGeometryResources(loaded_scene);
		updateMaterial(loaded_scene);
	}

	void SceneAdapter::setupNewScene(const std::shared_ptr<IScene> &scene) {
		createTlas();
		createUniformBuffers(scene);
	}

	void SceneAdapter::createTlas() {
		assert(top_level_acceleration_structure == nullptr);

		VkDevice device = vulkan_context->device_manager->getDevice();
		top_level_acceleration_structure = std::make_shared<AccelerationStructure>(
			device, *vulkan_context->resource_builder, *vulkan_context->command_manager,
			VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
		scene_resource_deletion_queue.pushFunction([&]()
		{
			top_level_acceleration_structure->destroy();
			top_level_acceleration_structure = nullptr;
		});
	}

	void SceneAdapter::createSceneLayout() {
		DescriptorLayoutBuilder layoutBuilder;

		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR); // TLAS
		layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render image
		layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // scene data
		layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // vertex buffer
		layoutBuilder.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // index buffer
		layoutBuilder.addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // geometry buffer
		layoutBuilder.addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // instance buffer
		layoutBuilder.addBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // emitting instances buffer
		layoutBuilder.addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6); // env map
		layoutBuilder.addBinding(9, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // rng tex

		scene_descriptor_set_layout = layoutBuilder.build(
				vulkan_context->device_manager->getDevice(),
				VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
		main_deletion_queue.pushFunction([&]() {
			vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), scene_descriptor_set_layout,
										 nullptr);
		});
	}

	void SceneAdapter::createSceneDescriptorSets(const VkDescriptorSetLayout &layout) {
		for (int i = 0; i < max_frames_in_flight; i++) {
			scene_descriptor_sets.push_back(vulkan_context->descriptor_allocator->allocate(
					vulkan_context->device_manager->getDevice(), layout));
		}
	}

	void SceneAdapter::createUniformBuffers(const std::shared_ptr<IScene> &scene) {
		sceneUniformBuffers.resize(max_frames_in_flight);
		sceneUniformBuffersMapped.resize(max_frames_in_flight);

		size_t scene_data_size = 0;
		scene->getSceneData(&scene_data_size, 0);
		for (size_t i = 0; i < max_frames_in_flight; i++) {
			VkDeviceSize size = scene_data_size;
			sceneUniformBuffers[i] = vulkan_context->resource_builder->createBuffer(
					size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			vkMapMemory(vulkan_context->device_manager->getDevice(), sceneUniformBuffers[i].bufferMemory, 0, size, 0,
						&sceneUniformBuffersMapped[i]);

			scene_resource_deletion_queue.pushFunction(
					[&, i]() { vulkan_context->resource_builder->destroyBuffer(sceneUniformBuffers[i]); });
		}
	}

	// ----------------------------------------------------------------------------------------------------------------

	void SceneAdapter::updateScene(const std::shared_ptr<DrawContext> &draw_context, uint32_t current_frame, uint32_t update_flags) {
		assert(loaded_scene != nullptr);

		// QuickTimer timer{"Scene Update", true};
		VkDevice device = vulkan_context->device_manager->getDevice();

		if (!(update_flags & NO_UPDATE))
			vkDeviceWaitIdle(device);

		if (update_flags & MATERIAL_UPDATE) {
			// !!!! This clear the descriptor set writes
			material_manager->updateMaterialResources(loaded_scene);
		}

		updateStaticGeometry(draw_context->getRenderObjects(), update_flags);
		updateSceneData(loaded_scene, draw_context, current_frame);

		updateSceneDescriptorSets();
	}

	void SceneAdapter::updateRenderTarget(const std::shared_ptr<RenderTarget> target) {
		vulkan_context->descriptor_allocator->writeImage(1, target->getCurrentTargetImage().imageView, VK_NULL_HANDLE,
														 VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		vulkan_context->descriptor_allocator->writeImage(9, target->getCurrentRngImage().imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
														 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		updateSceneDescriptorSets();
	}

	void SceneAdapter::updateGeometryResources(const std::shared_ptr<IScene> &scene) {
		std::vector<std::shared_ptr<MeshAsset>> mesh_assets = scene->getMeshAssets();
		geometry_manager->createGeometryBuffers(mesh_assets);
		geometry_manager->writeGeometryBuffers();
	}

	// TODO split into dynamic and static
	void SceneAdapter::updateStaticGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags) {
		if (update_flags & STATIC_GEOMETRY_UPDATE) {
			instance_manager->createInstanceMappingBuffer(render_objects);
			vulkan_context->descriptor_allocator->writeBuffer(6, instance_manager->getInstanceBuffer().handle, 0,
															  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

			// TODO Only partially update tlas depending on the updated dynamic objects
			updateTlas(render_objects);
			vulkan_context->descriptor_allocator->writeAccelerationStructure(
				0, top_level_acceleration_structure->getHandle(), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		}

		if (update_flags & STATIC_GEOMETRY_UPDATE || update_flags & MATERIAL_UPDATE) {
			instance_manager->createEmittingInstancesBuffer(render_objects);
			vulkan_context->descriptor_allocator->writeBuffer(7, instance_manager->getEmittingInstancesBuffer().handle,
												  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		}

		/*if (update_flags & GEOMETRY_UPDATE) {
			vulkan_context->descriptor_allocator->writeAccelerationStructure(
					0, top_level_acceleration_structure->getHandle(), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		}*/
	}

	void SceneAdapter::updateDynamicGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags) {

	}

	void SceneAdapter::updateTlas(std::vector<RenderObject> objects) const
	{
		uint32_t instance_id = 0;
		for (auto & object : objects) {
			top_level_acceleration_structure->addInstance(object.acceleration_structure, object.transform, instance_id++);
		}

		if (top_level_acceleration_structure->getHandle() == VK_NULL_HANDLE) {
			top_level_acceleration_structure->addInstanceGeometry();
		} else {
			top_level_acceleration_structure->update_instance_geometry(0);
		}
		top_level_acceleration_structure->build();
	}

	void SceneAdapter::updateMaterial(const std::shared_ptr<IScene> &scene) const {
		material_manager->updateMaterialResources(scene);
	}

	void SceneAdapter::updateSceneData(const std::shared_ptr<IScene> &scene, const std::shared_ptr<DrawContext> &draw_context, uint32_t current_frame) const {
		size_t size = 0;
		void* scene_data = scene->getSceneData(&size, draw_context->getEmittingObjectCount());
		memcpy(sceneUniformBuffersMapped[current_frame], scene_data, size);

		vulkan_context->descriptor_allocator->writeBuffer(2, sceneUniformBuffers[current_frame].handle, size,
														  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		scene->getEnvironmentMap()->writeToDescriptor(vulkan_context->descriptor_allocator, defaultSamplerLinear);
	}

	void SceneAdapter::updateSceneDescriptorSets() {
		VkDevice device = vulkan_context->device_manager->getDevice();
		for (int i = 0; i < max_frames_in_flight; i++) { // TODO das is doch schmarn
			vulkan_context->descriptor_allocator->updateSet(device, scene_descriptor_sets[i]);
		}
		vulkan_context->descriptor_allocator->clearWrites();
	}

	// -----------------------------------------------------------------------------------------------------------------

	void SceneAdapter::initDefaultResources(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) {
		createDefaultSamplers();
		createDefaultMaterials(raytracingProperties);
	}

	void SceneAdapter::createDefaultSamplers() {
		VkDevice device = vulkan_context->device_manager->getDevice();

		VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(vulkan_context->device_manager->getPhysicalDevice(), &properties);
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerAnisotropic) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		main_deletion_queue.pushFunction([&]() {
			vkDestroySampler(vulkan_context->device_manager->getDevice(), defaultSamplerLinear, nullptr);
			vkDestroySampler(vulkan_context->device_manager->getDevice(), defaultSamplerNearest, nullptr);
			vkDestroySampler(vulkan_context->device_manager->getDevice(), defaultSamplerAnisotropic, nullptr);
		});
	}

	void SceneAdapter::createDefaultMaterials(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) {
		auto phong_material = std::make_shared<PhongMaterial>(vulkan_context, texture_repository);
		phong_material->buildPipelines(scene_descriptor_set_layout);
		phong_material->pipeline->createShaderBindingTables(raytracingProperties);
		defaultMaterials["phong"] = phong_material;
		main_deletion_queue.pushFunction([&]() { defaultMaterials["phong"]->clearResources(); });

		auto metal_rough_material =
				std::make_shared<MetalRoughMaterial>(vulkan_context, texture_repository, defaultSamplerLinear);
		metal_rough_material->buildPipelines(scene_descriptor_set_layout);
		metal_rough_material->pipeline->createShaderBindingTables(raytracingProperties);
		defaultMaterials["metal_rough"] = metal_rough_material;
		main_deletion_queue.pushFunction([&]() { defaultMaterials["metal_rough"]->clearResources(); });
	}

	std::shared_ptr<Material> SceneAdapter::getMaterial() const { return loaded_scene->getMaterial(); }

	VkDescriptorSet SceneAdapter::getSceneDescriptorSet(uint32_t frame_index) const {
		return scene_descriptor_sets[frame_index];
	}

	void SceneAdapter::clearResources() {
		scene_resource_deletion_queue.flush();
		main_deletion_queue.flush();
	}
} // namespace RtEngine
