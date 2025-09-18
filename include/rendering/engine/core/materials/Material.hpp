#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <../../../builders/DescriptorAllocator.hpp>
#include <Pipeline.hpp>
#include <PropertiesManager.hpp>
#include <ResourceBuilder.hpp>
#include <utility>
#include <bits/shared_ptr.h>
#include <vulkan/vulkan_core.h>

#include "MaterialInstance.hpp"
#include "MaterialResourceManager.hpp"
#include "TextureRepository.hpp"

namespace RtEngine {
	class Pipeline;

	class Material : public ILayoutProvider {
	public:
		Material() = default;
		Material(std::string name, const std::shared_ptr<VulkanContext> &vulkan_context,
				 const std::shared_ptr<TextureRepository> &texture_repository) :
			name(std::move(name)), vulkan_context(vulkan_context), texture_repository(texture_repository) {
			std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
					{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
					{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			};
			descriptorAllocator.init(vulkan_context->device_manager->getDevice(), 64, poolRatios);

			mainDeletionQueue.pushFunction(
					[&]() { descriptorAllocator.destroyPools(this->vulkan_context->device_manager->getDevice()); });
		};

		virtual ~Material() = default;

		virtual void buildPipelines() = 0;

		virtual void addInstanceToResources(MaterialInstance &inst) = 0;

		virtual void writeMaterial() = 0;

		virtual glm::vec4 getEmissionForInstance([[maybe_unused]] uint32_t material_instance_id) { return glm::vec4(0.0f); }

		virtual std::vector<std::shared_ptr<Texture>> getTextures() = 0;

		std::vector<std::shared_ptr<MaterialInstance>> getInstances() {
			return instances;
		}

		virtual void *getPushConstants(uint32_t *out_size) = 0;
		// TODO Architecture: Phong needs to implement those too (those are only here because of push constants)
		virtual void resetSamples() {}
		virtual uint32_t getCurrSampleCount() { return 0; }
		virtual void progressSampleCount() {}
		virtual void setEmittingInstanceCount([[maybe_unused]] const uint32_t count) {}

		std::shared_ptr<PropertiesSection> getProperties() {
			if (properties == nullptr) {
				initProperties();
			}
			assert(properties != nullptr);
			return properties;
		}

		void destroyResources() {
			mainDeletionQueue.flush();
			destroyLayout();
		};

		virtual void reset() {
			// intentionally empty
		}

		std::string name;
		std::shared_ptr<Pipeline> pipeline;


	protected:
		virtual void initProperties() = 0;
		void destroyLayout() override {
			vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), descriptor_layout, nullptr);
		}

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<TextureRepository> texture_repository;
		DescriptorAllocator descriptorAllocator;
		DeletionQueue mainDeletionQueue;

		std::shared_ptr<PropertiesSection> properties;

		std::vector<std::shared_ptr<MaterialInstance>> instances;
	};

} // namespace RtEngine
#endif // MATERIAL_HPP
