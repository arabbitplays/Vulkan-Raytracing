#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <DescriptorAllocator.hpp>
#include <Pipeline.hpp>
#include <PropertiesManager.hpp>
#include <ResourceBuilder.hpp>
#include <bits/shared_ptr.h>
#include <vulkan/vulkan_core.h>

#include "TextureRepository.hpp"

namespace RtEngine {
	struct MaterialInstance;
	class Pipeline;

	class Material : public ILayoutProvider {
	public:
		Material() = default;
		Material(const std::string &name, const std::shared_ptr<VulkanContext> &vulkan_context,
				 const std::shared_ptr<TextureRepository> &texture_repository) :
			name(name), vulkan_context(vulkan_context), texture_repository(texture_repository) {
			std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
					{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
					{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			};
			descriptorAllocator.init(vulkan_context->device_manager->getDevice(), 64, poolRatios);

			mainDeletionQueue.pushFunction(
					[&]() { descriptorAllocator.destroyPools(this->vulkan_context->device_manager->getDevice()); });
		};

		virtual ~Material() = default;

		std::shared_ptr<Pipeline> pipeline;

		virtual void buildPipelines() = 0;
		virtual void writeMaterial() = 0;
		virtual glm::vec4 getEmissionForInstance([[maybe_unused]] uint32_t material_instance_id) { return glm::vec4(0.0f); }
		std::vector<std::shared_ptr<MaterialInstance>> getInstances();
		std::shared_ptr<PropertiesSection> getProperties();
		virtual std::vector<std::shared_ptr<Texture>> getTextures() = 0;

		void destroyResources();
		virtual void reset();

		std::string name;

	protected:
		virtual void initProperties() = 0;
		void destroyLayout() override;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<TextureRepository> texture_repository;
		DescriptorAllocator descriptorAllocator;
		DeletionQueue mainDeletionQueue, resetQueue;

		std::vector<std::shared_ptr<MaterialInstance>> instances;
		std::shared_ptr<PropertiesSection> properties;

		AllocatedBuffer material_buffer; // maps an instance to its respective material via a common index into the
										 // constants and texture buffers
	};

	struct MaterialInstance {
		std::shared_ptr<PropertiesSection> properties;
		uint32_t material_index;
	};

} // namespace RtEngine
#endif // MATERIAL_HPP
