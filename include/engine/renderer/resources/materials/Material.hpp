#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <DescriptorAllocator.hpp>
#include <../pipelines/RaytracingPipeline.hpp>
#include <ResourceBuilder.hpp>
#include <bits/shared_ptr.h>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

#include "ISerializable.hpp"
#include "MaterialInstance.hpp"

namespace RtEngine {
	class RaytracingPipeline;

	class Material : ISerializable {
	public:
		Material() = default;
		Material(std::string name, std::shared_ptr<VulkanContext> vulkan_context,
				 std::shared_ptr<TextureRepository> tex_repo) :
			name(name), vulkan_context(vulkan_context), tex_repo(tex_repo) {
			std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
					{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
					{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			};
			descriptorAllocator.init(vulkan_context->device_manager->getDevice(), 8, poolRatios);
			mainDeletionQueue.pushFunction(
					[&]() { descriptorAllocator.destroyPools(this->vulkan_context->device_manager->getDevice()); });
		};

		virtual ~Material() {};

		std::shared_ptr<RaytracingPipeline> pipeline;
		VkDescriptorSetLayout materialLayout;
		VkDescriptorSet materialDescriptorSet;

		virtual void buildPipelines(VkDescriptorSetLayout sceneLayout) = 0;
		virtual void writeMaterial(AllocatedBuffer material_buffer, std::shared_ptr<MaterialTextures<>> material_textures) = 0;

		virtual std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node) = 0;
		AllocatedBuffer createMaterialBuffer();

		std::vector<std::shared_ptr<MaterialInstance>> getInstances();

		std::shared_ptr<MaterialInstance> getInstanceByName(const std::string &name);

		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override = 0;
		virtual void getPushConstantValues(std::vector<int32_t>& push_constants) = 0;

		void clearResources();
		virtual void reset();

		std::string name;
	protected:
		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<TextureRepository> tex_repo;
		DescriptorAllocator descriptorAllocator;
		DeletionQueue mainDeletionQueue;

		std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> instances;
	};

} // namespace RtEngine
#endif // MATERIAL_HPP
