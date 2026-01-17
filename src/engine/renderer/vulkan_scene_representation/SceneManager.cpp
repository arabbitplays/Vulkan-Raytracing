#include "../../../../include/engine/renderer/vulkan_scene_representation/SceneManager.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <MeshRenderer.hpp>
#include <MetalRoughMaterial.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>
#include <SceneReader.hpp>
#include <SceneUtil.hpp>

namespace RtEngine {

	void SceneManager::createScene(const std::string& scene_path) {
		QuickTimer timer{"Scene Creation", true};

		if (scene != nullptr) {
			scene_resource_deletion_queue.flush();
		}

		SceneReader reader = SceneReader(vulkan_context, runtime_context);
		scene = reader.readScene(scene_path, defaultMaterials);

		scene_resource_deletion_queue.pushFunction([this] () {
			scene->clearResources();
		});

		setupNewScene(scene);
		updateGeometryResources(scene);
		updateMaterial(scene);

		bufferUpdateFlags = static_cast<uint8_t>(GEOMETRY_UPDATE) | static_cast<uint8_t>(MATERIAL_UPDATE);
	}

	void SceneManager::setupNewScene(const std::shared_ptr<Scene> &scene) {
		createNewTlas();
	}

	void SceneManager::createNewTlas() {
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

	void SceneManager::createSceneLayout() {
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

	void SceneManager::createSceneDescriptorSets(const VkDescriptorSetLayout &layout) {
		for (int i = 0; i < max_frames_in_flight; i++) {
			scene_descriptor_sets.push_back(vulkan_context->descriptor_allocator->allocate(
					vulkan_context->device_manager->getDevice(), layout));
		}
	}

	void SceneManager::createUniformBuffers() {
		sceneUniformBuffers.resize(max_frames_in_flight);
		sceneUniformBuffersMapped.resize(max_frames_in_flight);

		for (size_t i = 0; i < max_frames_in_flight; i++) {
			VkDeviceSize size = sizeof(SceneData);
			sceneUniformBuffers[i] = vulkan_context->resource_builder->createBuffer(
					size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			vkMapMemory(vulkan_context->device_manager->getDevice(), sceneUniformBuffers[i].bufferMemory, 0, size, 0,
						&sceneUniformBuffersMapped[i]);

			main_deletion_queue.pushFunction(
					[&, i]() { vulkan_context->resource_builder->destroyBuffer(sceneUniformBuffers[i]); });
		}
	}

	// ----------------------------------------------------------------------------------------------------------------

	void SceneManager::updateScene(const std::shared_ptr<DrawContext> &draw_context) {
		assert(scene != nullptr);

		// QuickTimer timer{"Scene Update", true};
		VkDevice device = vulkan_context->device_manager->getDevice();

		scene->update(vulkan_context->swapchain->extent.width, vulkan_context->swapchain->extent.height);

		if (!(bufferUpdateFlags & NO_UPDATE))
			vkDeviceWaitIdle(device);

		draw_context->clear();

		scene->nodes["root"]->draw(*draw_context);
		if (bufferUpdateFlags != NO_UPDATE) {
			if (bufferUpdateFlags & MATERIAL_UPDATE) {
				// !!!! This clear the descriptor set writes
				material_manager->updateMaterialResources(scene);
			}
		}

		updateStaticGeometry(draw_context->getRenderObjects(), bufferUpdateFlags);
		updateSceneData(draw_context);


		// TODO Only partially update tlas depending on the updated dynamic objects

		updateSceneDescriptorSets(draw_context);

		bufferUpdateFlags = NO_UPDATE;
	}

	void SceneManager::updateGeometryResources(const std::shared_ptr<Scene> &scene) {
		std::vector<std::shared_ptr<MeshAsset>> mesh_assets = SceneUtil::collectMeshAssets(scene->getRootNode());
		geometry_manager->createGeometryBuffers(mesh_assets);
		geometry_manager->writeGeometryBuffers();
	}

	// TODO split into dynamic and static
	void SceneManager::updateStaticGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags) {
		if (bufferUpdateFlags & GEOMETRY_UPDATE) {
			instance_manager->createInstanceMappingBuffer(render_objects);
			updateTlas(render_objects);
		}

		if (bufferUpdateFlags & GEOMETRY_UPDATE || bufferUpdateFlags & MATERIAL_UPDATE) {
			instance_manager->createEmittingInstancesBuffer(render_objects);
			vulkan_context->descriptor_allocator->writeBuffer(7, instance_manager->getEmittingInstancesBuffer().handle,
												  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		}

		/*if (update_flags & GEOMETRY_UPDATE) {
			vulkan_context->descriptor_allocator->writeAccelerationStructure(
					0, top_level_acceleration_structure->getHandle(), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		}*/
	}

	void SceneManager::updateDynamicGeometry(std::vector<RenderObject> render_objects, uint32_t update_flags) {
		// TODO This still update all objects in the TLAS

	}

	void SceneManager::updateTlas(std::vector<RenderObject> objects) const
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

	void SceneManager::updateMaterial(const std::shared_ptr<Scene> &scene) const {
		material_manager->updateMaterialResources(scene);
	}

	void SceneManager::updateSceneData(const std::shared_ptr<DrawContext> &draw_context) {
		std::shared_ptr<SceneData> scene_data = scene->createSceneData(draw_context->getEmittingObjectCount());
		memcpy(sceneUniformBuffersMapped[draw_context->currentFrame], scene_data.get(), sizeof(SceneData));

		vulkan_context->descriptor_allocator->writeBuffer(2, sceneUniformBuffers[draw_context->currentFrame].handle, sizeof(SceneData),
														  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}

	void SceneManager::updateSceneDescriptorSets(const std::shared_ptr<DrawContext> &draw_context) {
		VkDevice device = vulkan_context->device_manager->getDevice();

		VkAccelerationStructureKHR tlas_handle = top_level_acceleration_structure->getHandle();
		if (bufferUpdateFlags & GEOMETRY_UPDATE) {
			//TODO move those two to updateStaticGeometry
			vulkan_context->descriptor_allocator->writeAccelerationStructure(
					0, tlas_handle, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

			vulkan_context->descriptor_allocator->writeBuffer(6, instance_manager->getInstanceBuffer().handle, 0,
															  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			std::vector<VkImageView> views{};
			for (uint32_t i = 0; i < 6; i++) {
				views.push_back(scene->environment_map[i].imageView);
			}
			vulkan_context->descriptor_allocator->writeImages(8, views, defaultSamplerLinear,
															  VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
															  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		vulkan_context->descriptor_allocator->writeImage(1, draw_context->target->getCurrentTargetImage().imageView, VK_NULL_HANDLE,
														 VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		vulkan_context->descriptor_allocator->writeImage(9, draw_context->target->getCurrentRngImage().imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
														 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		for (int i = 0; i < max_frames_in_flight; i++) { // TODO das is doch schmarn
			vulkan_context->descriptor_allocator->updateSet(device, scene_descriptor_sets[i]);
		}
		vulkan_context->descriptor_allocator->clearWrites();
	}

	// -----------------------------------------------------------------------------------------------------------------

	void SceneManager::initDefaultResources(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) {
		createDefaultSamplers();
		createDefaultMaterials(raytracingProperties);
	}

	void SceneManager::createDefaultSamplers() {
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

	void SceneManager::createDefaultMaterials(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) {
		auto phong_material = std::make_shared<PhongMaterial>(vulkan_context, runtime_context);
		phong_material->buildPipelines(scene_descriptor_set_layout);
		phong_material->pipeline->createShaderBindingTables(raytracingProperties);
		defaultMaterials["phong"] = phong_material;
		main_deletion_queue.pushFunction([&]() { defaultMaterials["phong"]->clearResources(); });

		auto metal_rough_material =
				std::make_shared<MetalRoughMaterial>(vulkan_context, runtime_context, defaultSamplerLinear);
		metal_rough_material->buildPipelines(scene_descriptor_set_layout);
		metal_rough_material->pipeline->createShaderBindingTables(raytracingProperties);
		defaultMaterials["metal_rough"] = metal_rough_material;
		main_deletion_queue.pushFunction([&]() { defaultMaterials["metal_rough"]->clearResources(); });
	}

	std::shared_ptr<Material> SceneManager::getMaterial() const { return scene->material; }

	VkDescriptorSet SceneManager::getSceneDescriptorSet(uint32_t frame_index) const {
		return scene_descriptor_sets[frame_index];
	}

	SceneManager::SceneInfo SceneManager::getSceneInformation() const
	{
		SceneInfo scene_info {
			.path = scene != nullptr ? scene->path : "",
		};
		return scene_info;
	}

	void SceneManager::clearResources() {
		scene_resource_deletion_queue.flush();
		main_deletion_queue.flush();
	}
} // namespace RtEngine
