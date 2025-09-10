#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <Material.hpp>
#include "MetalRoughInstance.hpp"

#define METAL_ROUGH_MATERIAL_NAME "metal_rough"

namespace RtEngine {
	class MetalRoughMaterial : public Material {
	public:
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

		struct MaterialProperties {
			int32_t normal_mapping = 0, sample_lights = 0, sample_bsdf = 0, russian_roulette = 0;
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

		std::shared_ptr<MaterialInstance> createInstance(const MetalRoughInstance::Parameters &parameters);

		void addInstanceToResources(MaterialInstance &inst) override;

		void addInstanceToResources(MetalRoughInstance &instance);

		void reset() override;

	protected:
		void initProperties() override;

		VkDescriptorSetLayout createLayout() override;
		std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) override;

		std::shared_ptr<MetalRoughResources> mapInstanceToResources(const MetalRoughInstance &instance) const;

	private:
		std::shared_ptr<MaterialResourceManager<MetalRoughResources>> resource_manager;

		MaterialProperties material_properties;
		VkSampler sampler;
	};

} // namespace RtEngine
#endif // METALROUGHMATERIAL_HPP
