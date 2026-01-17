#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <Material.hpp>
#include <TextureRepository.hpp>
#include <glm/vec3.hpp>

#define METAL_ROUGH_MATERIAL_NAME "metal_rough"

namespace RtEngine {
	struct MetalRoughParameters {

	};

	class MetalRoughMaterial : public Material {
	public:


		struct MaterialProperties {
			int32_t normal_mapping = 0, sample_lights = 0, sample_bsdf = 0, russian_roulette = 0;
		};

		MetalRoughMaterial(std::shared_ptr<VulkanContext> context, std::shared_ptr<RuntimeContext> runtime_context,
						   VkSampler sampler) :
			Material(METAL_ROUGH_MATERIAL_NAME, context, runtime_context), sampler(sampler) {
			material_texture_repo = std::make_shared<TextureRepository<>>(vulkan_context->resource_builder);
			mainDeletionQueue.pushFunction([this] () {
				material_texture_repo->destroy();
			});
		}

		void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
		void writeMaterial() override;
		std::vector<std::shared_ptr<Texture>> getTextures() override;

		std::shared_ptr<MaterialInstance> createInstance(MetalRoughParameters parameters, bool unique = false);

		std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node);

		void reset() override;

	protected:
		void initProperties() override;

	private:
		AllocatedBuffer createMaterialBuffer();

		std::shared_ptr<TextureRepository<>> material_texture_repo;

		MaterialProperties material_properties;
		VkSampler sampler;
	};

} // namespace RtEngine
#endif // METALROUGHMATERIAL_HPP
