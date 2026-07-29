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

		void ensurePipelineSpecialization(uint32_t required_depth, uint32_t mlmc_method, bool do_mlmc) override;

		std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node) override;

		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;
		void getPushConstantValues(std::vector<int32_t> &push_constants) override;

		void reset() override;

	private:
		// Specialization state currently baked into `pipeline`. Kept in sync
		// with the shader-side spec constants in shaders/metalRough/options.glsl
		// (plus MAX_PATH_LENGTH in shaders/common/path.glsl). Any mismatch with
		// the values requested by ensurePipelineSpecialization triggers a rebuild.
		uint32_t max_path_length = 32;                // constant_id 0
		uint32_t current_mlmc_method = 0;             // constant_id 1
		bool current_do_mlmc = false;                 // constant_id 2
		bool current_sample_bsdf = false;             // constant_id 3
		bool current_russian_roulette = false;        // constant_id 4
		bool pipeline_specialized = false;
		VkDescriptorSetLayout scene_layout = VK_NULL_HANDLE;

		bool normal_mapping = false, sample_lights = false, sample_bsdf = false, russian_roulette = false, similarity_relation = false, use_first_order_similarity = false, assume_homogenous = false, regular_tracking = false;
		bool debug_depth = false;
		bool adaptive_sampling = false;
		bool debug_variance = false, debug_diff_variance = false;
		bool debug_adaptive_sampling = false;
		float adaptive_error_bound = 0.05f;
		int32_t adaptive_min_samples = 16;

		VkSampler sampler;
	};

} // namespace RtEngine
#endif // METALROUGHMATERIAL_HPP
