#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <Material.hpp>
#include <MaterialTextures.hpp>
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
			Material(METAL_ROUGH_MATERIAL_NAME, context, runtime_context), sampler(sampler) {}

		void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
		void writeMaterial(AllocatedBuffer material_buffer, std::shared_ptr<MaterialTextures<>> material_textures) override;

		std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node) override;

		void reset() override;

	protected:
		void initProperties() override;

	private:

		MaterialProperties material_properties;
		VkSampler sampler;
	};

} // namespace RtEngine
#endif // METALROUGHMATERIAL_HPP
