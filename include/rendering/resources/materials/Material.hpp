#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <DescriptorAllocator.hpp>
#include <Pipeline.hpp>
#include <PropertiesManager.hpp>
#include <ResourceBuilder.hpp>
#include <RuntimeContext.hpp>
#include <bits/shared_ptr.h>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

#include "MaterialInstance.hpp"

namespace RtEngine {
	class Pipeline;

	class Material {
	public:
		Material() = default;
		Material(std::string name, std::shared_ptr<VulkanContext> vulkan_context,
				 std::shared_ptr<RuntimeContext> runtime_context) :
			name(name), vulkan_context(vulkan_context), runtime_context(runtime_context) {
			std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
					{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
					{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			};
			descriptorAllocator.init(vulkan_context->device_manager->getDevice(), 8, poolRatios);

			mainDeletionQueue.pushFunction(
					[&]() { descriptorAllocator.destroyPools(this->vulkan_context->device_manager->getDevice()); });
		};

		virtual ~Material() {};

		std::shared_ptr<Pipeline> pipeline;
		VkDescriptorSetLayout materialLayout;
		VkDescriptorSet materialDescriptorSet;

		virtual void buildPipelines(VkDescriptorSetLayout sceneLayout) = 0;
		virtual void writeMaterial() = 0;
		std::vector<std::shared_ptr<MaterialInstance>> getInstances();
		std::shared_ptr<PropertiesSection> getProperties();
		virtual std::vector<std::shared_ptr<Texture>> getTextures() = 0;

		void clearRessources();
		virtual void reset();

		std::string name;

	protected:
		virtual void initProperties() = 0;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<RuntimeContext> runtime_context;
		DescriptorAllocator descriptorAllocator;
		DeletionQueue mainDeletionQueue, resetQueue;

		std::vector<std::shared_ptr<MaterialInstance>> instances;
		std::shared_ptr<PropertiesSection> properties;

		AllocatedBuffer material_buffer; // maps an instance to its respective material via a common index into the
										 // constants and texture buffers
	};

} // namespace RtEngine
#endif // MATERIAL_HPP
