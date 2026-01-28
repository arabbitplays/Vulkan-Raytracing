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
		MetalRoughMaterial(std::shared_ptr<VulkanContext> context, std::shared_ptr<TextureRepository> tex_repo,
						   VkSampler sampler) :
			Material(METAL_ROUGH_MATERIAL_NAME, context, tex_repo), sampler(sampler) {}

		void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
		void writeMaterial(AllocatedBuffer material_buffer, std::shared_ptr<MaterialTextures<>> material_textures) override;

		std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node) override;

		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;
		void getPushConstantValues(std::vector<int32_t> &push_constants) override;

		void reset() override;

	private:

		bool normal_mapping = false, sample_lights = false, sample_bsdf = false, russian_roulette = false;

		VkSampler sampler;
	};

} // namespace RtEngine
#endif // METALROUGHMATERIAL_HPP
