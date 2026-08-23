#include "MetalRoughMaterial.hpp"

#include <cstring>
#include <DescriptorLayoutBuilder.hpp>
#include <DeviceManager.hpp>
#include <OptionsWindow.hpp>
#include <VulkanUtil.hpp>
#include <spdlog/spdlog.h>
#include <metal_rough_closesthit.rchit.spv.h>
#include <shadow_clostesthit.rchit.spv.h>
#include <metal_rough_miss.rmiss.spv.h>
#include <metal_rough_raygen.rgen.spv.h>
#include <shadow_miss.rmiss.spv.h>

#include "MetalRoughInstance.hpp"

namespace RtEngine {
	void MetalRoughMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
		VkDevice device = vulkan_context->device_manager->getDevice();
		scene_layout = sceneLayout;

		// Layout, descriptor set and deletion-queue entries are created once;
		// the pipeline itself can be rebuilt (see ensurePipelineSpecialization).
		if (materialLayout == VK_NULL_HANDLE) {
			DescriptorLayoutBuilder layoutBuilder;
			layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64); // TODO make this dynamic depending on the scene

			materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
			mainDeletionQueue.pushFunction([&]() {
				vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), materialLayout, nullptr);
			});
			materialDescriptorSet = descriptorAllocator.allocate(device, materialLayout);
			mainDeletionQueue.pushFunction([&]() {
				if (pipeline)
					pipeline->destroy();
			});
		}

		if (pipeline) { // rebuild: the caller must have waited for device idle
			pipeline->destroy();
		}
		pipeline = std::make_shared<RaytracingPipeline>(vulkan_context);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{scene_layout, materialLayout};
		pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

		// Specialization constants — see shaders/metalRough/options.glsl and
		// shaders/common/path.glsl. Caller (ensurePipelineSpecialization or
		// the initial buildPipelines) sets current_* first; here we just bake.
		current_sample_bsdf = sample_bsdf;
		current_russian_roulette = russian_roulette;
		pipeline_specialized = true;
		pipeline->setSpecConstant(0, max_path_length);            // MAX_PATH_LENGTH
		pipeline->setSpecConstant(1, current_mlmc_method);        // SPEC_MLMC_METHOD
		pipeline->setSpecConstant(2, current_do_mlmc ? 1u : 0u);  // SPEC_DO_MLMC
		pipeline->setSpecConstant(3, current_sample_bsdf ? 1u : 0u);        // SPEC_SAMPLE_BSDF
		pipeline->setSpecConstant(4, current_russian_roulette ? 1u : 0u);   // SPEC_RUSSIAN_ROULETTE

		pipeline->addPushConstant(21 * sizeof(uint32_t), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
																  VK_SHADER_STAGE_RAYGEN_BIT_KHR |
																  VK_SHADER_STAGE_MISS_BIT_KHR);

		VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(
				device, oschd_metal_rough_raygen_rgen_spv_size(), oschd_metal_rough_raygen_rgen_spv());
		VkShaderModule missShaderModule = VulkanUtil::createShaderModule(
				device, oschd_metal_rough_miss_rmiss_spv_size(), oschd_metal_rough_miss_rmiss_spv());
		VkShaderModule shadowMissShaderModule = VulkanUtil::createShaderModule(
				device, oschd_shadow_miss_rmiss_spv_size(), oschd_shadow_miss_rmiss_spv());
		VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(
				device, oschd_metal_rough_closesthit_rchit_spv_size(), oschd_metal_rough_closesthit_rchit_spv());
		VkShaderModule shadow_hit_shader_module = VulkanUtil::createShaderModule(
				device, oschd_shadow_clostesthit_rchit_spv_size(), oschd_shadow_clostesthit_rchit_spv());

		pipeline->addShaderStage(raygenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
		pipeline->addShaderStage(missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
		pipeline->addShaderStage(shadowMissShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
		pipeline->addShaderStage(closestHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);
		pipeline->addShaderStage(shadow_hit_shader_module, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);

		pipeline->build();

		vkDestroyShaderModule(device, raygenShaderModule, nullptr);
		vkDestroyShaderModule(device, missShaderModule, nullptr);
		vkDestroyShaderModule(device, shadowMissShaderModule, nullptr);
		vkDestroyShaderModule(device, closestHitShaderModule, nullptr);
		vkDestroyShaderModule(device, shadow_hit_shader_module, nullptr);
	}

	void MetalRoughMaterial::ensurePipelineSpecialization(uint32_t required_depth, uint32_t mlmc_method, bool do_mlmc) {
		if (scene_layout == VK_NULL_HANDLE) {
			return;
		}

		// Grow path capacity monotonically, rounded up so sweeping the depth
		// upwards does not rebuild every step.
		uint32_t new_path_length = max_path_length;
		if (required_depth > max_path_length) {
			new_path_length = (required_depth + 15u) / 16u * 16u;
		}

		const bool needs_rebuild = !pipeline_specialized
			|| new_path_length != max_path_length
			|| mlmc_method != current_mlmc_method
			|| do_mlmc != current_do_mlmc
			|| sample_bsdf != current_sample_bsdf
			|| russian_roulette != current_russian_roulette;
		if (!needs_rebuild) {
			return;
		}

		spdlog::info(
			"Rebuilding metal-rough pipeline: path_length {}->{}, mlmc_method {}->{}, do_mlmc {}->{}, sample_bsdf {}->{}, rr {}->{}",
			max_path_length, new_path_length,
			current_mlmc_method, mlmc_method,
			current_do_mlmc, do_mlmc,
			current_sample_bsdf, sample_bsdf,
			current_russian_roulette, russian_roulette);

		max_path_length = new_path_length;
		current_mlmc_method = mlmc_method;
		current_do_mlmc = do_mlmc;

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
		buildPipelines(scene_layout);
		pipeline->createShaderBindingTables(DeviceManager::RAYTRACING_PROPERTIES);
	}

	void MetalRoughMaterial::writeMaterial(AllocatedBuffer material_buffer, std::shared_ptr<MaterialTextures<>> material_textures) {
		descriptorAllocator.writeBuffer(0, material_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		descriptorAllocator.writeImages(1, material_textures->getOrderedImageViews(), sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
										VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		VkDevice device = vulkan_context->device_manager->getDevice();
		descriptorAllocator.updateSet(device, materialDescriptorSet);
		descriptorAllocator.clearWrites();
	}

	std::shared_ptr<MaterialInstance> MetalRoughMaterial::loadInstance(const YAML::Node& yaml_node) {
		std::shared_ptr<MaterialInstance> instance = std::make_shared<MetalRoughInstance>("", tex_repo);
		instance->loadResources(yaml_node);
		if (instances.contains(instance->name)) {
			SPDLOG_WARN("Material instance with name {} already exists (but will be overwritten)!", instance->name);
		}

		instances[instance->name] = instance;
		return instance;
	}

	void MetalRoughMaterial::initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) {
		bool reset_required = false;
		if (config->startChild(name)) {
			const bool prev_similarity = similarity_relation;
			const bool prev_first_order = use_first_order_similarity;
			reset_required |= config->addBool("normal_mapping", &normal_mapping);
			reset_required |= config->addBool("next_event_estimation", &sample_lights);
			reset_required |= config->addBool("bsdf_importance_sampling", &sample_bsdf);
			reset_required |= config->addBool("russian_roulette", &russian_roulette);
			reset_required |= config->addBool("similarity_relation", &similarity_relation);
			reset_required |= config->addBool("use_first_order_similarity", &use_first_order_similarity);
			// Normal and first-order similarity are mutually exclusive; whichever
			// flipped this frame wins and clears the other.
			if (similarity_relation && use_first_order_similarity) {
				if (similarity_relation != prev_similarity) {
					use_first_order_similarity = false;
				} else if (use_first_order_similarity != prev_first_order) {
					similarity_relation = false;
				} else {
					use_first_order_similarity = false;
				}
			}
			reset_required |= config->addBool("assume_homogenous", &assume_homogenous);
			reset_required |= config->addBool("regular_tracking", &regular_tracking);
			reset_required |= config->addBool("debug_depth", &debug_depth);
			reset_required |= config->addBool("adaptive_sampling", &adaptive_sampling);
			reset_required |= config->addBool("debug_variance", &debug_variance);
			reset_required |= config->addBool("debug_diff_variance", &debug_diff_variance);
			reset_required |= config->addBool("debug_adaptive_sampling", &debug_adaptive_sampling);
			reset_required |= config->addBool("debug_convergence_ratio", &debug_convergence_ratio);
			reset_required |= config->addFloat("adaptive_error_bound", &adaptive_error_bound, 0.001f, 1.0f);
			reset_required |= config->addInt("adaptive_min_samples", &adaptive_min_samples, 2, 4096);
			config->endChild();
		}

		if (reset_required) {
			update_flags->setFlag(TARGET_RESET);
		}
	}

	void MetalRoughMaterial::getPushConstantValues(std::vector<int32_t> &push_constants) {
		push_constants.push_back(static_cast<int32_t>(normal_mapping));
		push_constants.push_back(static_cast<int32_t>(sample_lights));
		// sample_bsdf and russian_roulette are specialization constants now
		// (see MetalRoughMaterial::ensurePipelineSpecialization).
		push_constants.push_back(static_cast<int32_t>(similarity_relation));
		push_constants.push_back(static_cast<int32_t>(use_first_order_similarity));
		push_constants.push_back(static_cast<int32_t>(assume_homogenous));
		push_constants.push_back(static_cast<int32_t>(regular_tracking));
		push_constants.push_back(static_cast<int32_t>(debug_depth));
		push_constants.push_back(static_cast<int32_t>(adaptive_sampling));
		push_constants.push_back(static_cast<int32_t>(debug_variance));
		push_constants.push_back(static_cast<int32_t>(debug_diff_variance));
		push_constants.push_back(static_cast<int32_t>(debug_adaptive_sampling));
		push_constants.push_back(static_cast<int32_t>(debug_convergence_ratio));
		int32_t error_bound_bits;
		std::memcpy(&error_bound_bits, &adaptive_error_bound, sizeof(float));
		push_constants.push_back(error_bound_bits);
		push_constants.push_back(adaptive_min_samples);
	}

	void MetalRoughMaterial::reset() {
		instances.clear();
		Material::reset();
	}
} // namespace RtEngine
