#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <../../engine/core/materials/Material.hpp>

#include "DescriptorLayoutBuilder.hpp"
#include "MetalRoughInstance.hpp"

#define METAL_ROUGH_MATERIAL_NAME "metal_rough"

namespace RtEngine {
	class MetalRoughMaterial : public Material {
	public:

		// the data that is written to the material buffer
		struct MetalRoughResources : MaterialResources {
			glm::vec3 albedo;
			float padding;
			glm::vec4 properties; // metallic roughness ao eta
			glm::vec4 emission;
			glm::ivec4 tex_indices; // albedo, metal_rough_ao, normal
			bool operator==(const MetalRoughResources &other) const {
				return glm::all(glm::epsilonEqual(albedo, other.albedo, 0.0001f)) &&
					   glm::all(glm::epsilonEqual(properties, other.properties, 0.0001f)) &&
					   glm::all(glm::epsilonEqual(emission, other.emission, 0.0001f)) &&
					   glm::all(glm::equal(tex_indices, other.tex_indices));
			}
		};


		struct PushConstants {
			int32_t recursion_depth = 3;
			int32_t normal_mapping = 0, sample_lights = 0, sample_bsdf = 0, russian_roulette = 0;
			uint32_t curr_sample_count = 0;
			uint32_t emitting_instances_count = 0;
			int32_t samples_per_pixel = 1;
		};

		MetalRoughMaterial(const std::shared_ptr<VulkanContext> &context, const std::shared_ptr<TextureRepository> &texture_repository,
						   VkSampler sampler) :
			Material(METAL_ROUGH_MATERIAL_NAME, context, texture_repository), sampler(sampler) {

			resource_manager = std::make_shared<MaterialResourceManager<MetalRoughResources>>(vulkan_context);
			mainDeletionQueue.pushFunction([&]() {
				resource_manager->destroyResources();
			});
		}

		void buildPipelines() override;
		void writeMaterial() override;
		glm::vec4 getEmissionForInstance(uint32_t material_instance_id) override;
		std::vector<std::shared_ptr<MetalRoughResources>> getResources() const;
		std::vector<std::shared_ptr<Texture>> getTextures() override;

		void* getPushConstants(uint32_t *out_size) override;
		void resetSamples() override;
		void setEmittingInstanceCount(const uint32_t count) override;
		uint32_t getCurrSampleCount() override;
		void progressSampleCount() override;

		std::shared_ptr<MaterialInstance> createInstance(const MetalRoughInstance::Parameters &parameters);

		void addInstanceToResources(MaterialInstance &inst) override;
		void addInstanceToResources(MetalRoughInstance &instance);

		void reset() override;

	protected:
		void initProperties() override;

		VkDescriptorSetLayout createLayout() override;
		void defineTextureLayout(DescriptorLayoutBuilder &layout_builder, uint32_t binding_idx);
		std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) override;

		std::shared_ptr<MetalRoughResources> mapInstanceToResources(const MetalRoughInstance &instance) const;

	private:
		std::shared_ptr<MaterialResourceManager<MetalRoughResources>> resource_manager;

		PushConstants push_constants;
		VkSampler sampler;
	};

} // namespace RtEngine
#endif // METALROUGHMATERIAL_HPP
