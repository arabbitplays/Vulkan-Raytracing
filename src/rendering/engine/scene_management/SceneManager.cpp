#include "../../../../include/rendering/engine/scene_management/SceneManager.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <MeshRenderer.hpp>
#include <MetalRoughMaterial.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>
#include <SceneReader.hpp>
#include <SceneUtil.hpp>

namespace RtEngine {

	SceneManager::SceneManager(const std::shared_ptr<VulkanContext> &vulkanContext,
				const std::shared_ptr<RuntimeContext> &runtime_context,
				const uint32_t max_frames_in_flight)
				: vulkan_context(vulkanContext), runtime_context(runtime_context),
				max_frames_in_flight(max_frames_in_flight) {
		geometry_manager = std::make_shared<GeometryManager>(vulkan_context->resource_builder);
		instance_manager = std::make_shared<InstanceManager>(vulkan_context->resource_builder);

		main_deletion_queue.pushFunction([&]() {
			geometry_manager->destroy();
			instance_manager->destroy();
		});

		initLayout();
	}


	void SceneManager::createScene(const std::string& scene_path) {
		QuickTimer timer{"Scene Creation", true};

		if (scene != nullptr) {
			scene_resource_deletion_queue.flush();
			runtime_context->material_repository->resetMaterials();
		}

		SceneReader reader = SceneReader(vulkan_context, runtime_context);
		scene = reader.readScene(scene_path);

		scene_resource_deletion_queue.pushFunction([&]() { scene->clearResources(); });

		std::vector<std::shared_ptr<MeshAsset>> mesh_assets = SceneUtil::collectMeshAssets(scene->getRootNode());
		geometry_manager->createGeometryBuffers(mesh_assets);
		scene->material->writeMaterial();
		createBlas(mesh_assets);
		createUniformBuffers();
		bufferUpdateFlags = static_cast<uint8_t>(GEOMETRY_UPDATE) | static_cast<uint8_t>(MATERIAL_UPDATE);

		VkDevice device = vulkan_context->device_manager->getDevice();
		top_level_acceleration_structure = std::make_shared<AccelerationStructure>(
			device, *vulkan_context->resource_builder, *vulkan_context->command_manager,
			VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
		scene_resource_deletion_queue.pushFunction([&]()
		{
			top_level_acceleration_structure->destroy();
		});
	}

	void SceneManager::createBlas(const std::vector<std::shared_ptr<MeshAsset>> &meshes) {
		QuickTimer timer{"BLAS Build", true};
		VkDevice device = vulkan_context->device_manager->getDevice();

		uint32_t object_id = 0;
		for (auto &meshAsset: meshes) {
			meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(
					device, *vulkan_context->resource_builder, *vulkan_context->command_manager,
					VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

			meshAsset->accelerationStructure->addTriangleGeometry(
					geometry_manager->getVertexBuffer(), geometry_manager->getIndexBuffer(),
					meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
					meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
			meshAsset->accelerationStructure->build();
			meshAsset->geometry_id = object_id++;
		}

		scene_resource_deletion_queue.pushFunction([&]() {
			for (auto &meshAsset: SceneUtil::collectMeshAssets(scene->getRootNode())) {
				meshAsset->accelerationStructure->destroy();
			}
		});
	}


	VkDescriptorSetLayout SceneManager::createLayout() {
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

		return layoutBuilder.build(
				vulkan_context->device_manager->getDevice(),
				VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
	}

	std::shared_ptr<DescriptorSet> SceneManager::createDescriptorSet(const VkDescriptorSetLayout &layout) {
		return std::make_shared<DescriptorSet>(vulkan_context->descriptor_allocator, vulkan_context->device_manager, layout, max_frames_in_flight);
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

			scene_resource_deletion_queue.pushFunction(
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

		draw_context->objects.clear();
		scene->nodes["root"]->draw(*draw_context);

		if (bufferUpdateFlags & GEOMETRY_UPDATE) {
			instance_manager->createInstanceMappingBuffer(draw_context->objects);
		}

		if (bufferUpdateFlags & GEOMETRY_UPDATE || bufferUpdateFlags & MATERIAL_UPDATE) {
			instance_manager->createEmittingInstancesBuffer(draw_context->objects, getMaterial());
			vulkan_context->descriptor_allocator->writeBuffer(7, instance_manager->getEmittingInstancesBuffer().handle,
												  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		}

		if (bufferUpdateFlags != NO_UPDATE) {
			if (bufferUpdateFlags & MATERIAL_UPDATE) {
				scene->material->writeMaterial();
			}
		}

		// TODO Only partially update tlas depending on the updated dynamic objects
		updateTlas(draw_context->objects);

		std::shared_ptr<SceneData> scene_data = scene->createSceneData();
		memcpy(sceneUniformBuffersMapped[draw_context->currentFrame], scene_data.get(), sizeof(SceneData));

		updateSceneDescriptorSets(draw_context->currentFrame, draw_context->target);

		bufferUpdateFlags = NO_UPDATE;
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

	void SceneManager::updateSceneDescriptorSets(uint32_t current_frame, const std::shared_ptr<RenderTarget>& render_target) const {
		VkDevice device = vulkan_context->device_manager->getDevice();

		VkAccelerationStructureKHR tlas_handle = top_level_acceleration_structure->getHandle();
		if (bufferUpdateFlags & GEOMETRY_UPDATE) {
			vulkan_context->descriptor_allocator->writeAccelerationStructure(
					0, tlas_handle, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
			vulkan_context->descriptor_allocator->writeBuffer(3, geometry_manager->getVertexBuffer().handle, 0,
															  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vulkan_context->descriptor_allocator->writeBuffer(4, geometry_manager->getIndexBuffer().handle, 0,
															  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vulkan_context->descriptor_allocator->writeBuffer(5, geometry_manager->getGeometryMappingBuffer().handle, 0,
															  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vulkan_context->descriptor_allocator->writeBuffer(6, instance_manager->getInstanceBuffer().handle, 0,
															  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);


			std::vector<VkImageView> views{};
			for (uint32_t i = 0; i < 6; i++) {
				views.push_back(scene->environment_map[i].imageView);
			}
			vulkan_context->descriptor_allocator->writeImages(8, views, runtime_context->default_resources->getLinearSampler(),
															  VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
															  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		vulkan_context->descriptor_allocator->writeImage(1, render_target->getCurrentTargetImage().imageView, VK_NULL_HANDLE,
														 VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		vulkan_context->descriptor_allocator->writeBuffer(2, sceneUniformBuffers[current_frame].handle, sizeof(SceneData),
														  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		vulkan_context->descriptor_allocator->writeImage(9, render_target->getCurrentRngImage().imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
														 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		for (uint32_t i = 0; i < max_frames_in_flight; i++) {
			vulkan_context->descriptor_allocator->updateSet(device, descriptor_set->getCurrentSet(i));
		}
		vulkan_context->descriptor_allocator->clearWrites();
	}

	// -----------------------------------------------------------------------------------------------------------------

	std::shared_ptr<Material> SceneManager::getMaterial() const { return scene->material; }

	SceneManager::SceneInfo SceneManager::getSceneInformation() const
	{
		SceneInfo scene_info {
			.path = scene != nullptr ? scene->path : "",
			.emitting_instances_count = instance_manager->getEmittingInstancesCount()
		};
		return scene_info;
	}

	void SceneManager::destroyResources() {
		scene_resource_deletion_queue.flush();
		main_deletion_queue.flush();
		destroyLayout();
	}

	void SceneManager::destroyLayout() {
		vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), descriptor_layout,
								 nullptr);
	}
} // namespace RtEngine
