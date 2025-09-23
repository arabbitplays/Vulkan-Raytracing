#ifndef PHONGMATERIAL_HPPPipeline
#define PHONGMATERIAL_HPP
#include <../../engine/core/materials/Material.hpp>

#include "PhongInstance.hpp"

#define PHONG_MATERIAL_NAME "phong"

namespace RtEngine {
	class PhongMaterial : public Material {
	public:
		struct PhongResources : MaterialResources {
			glm::vec3 diffuse;
			glm::vec3 specular;
			glm::vec3 ambient;
			glm::vec3 reflection;
			glm::vec3 transmission;
			float n;
			glm::vec4 eta; // only xyz for the eta of each rgb channel
		};

		struct PushConstants {
			int32_t recursion_depth = 3;
			int32_t shadows = false, dispersion = false, fresnel = false;
		};

		PhongMaterial(const std::shared_ptr<VulkanContext> &context, const std::shared_ptr<TextureRepository> &texture_repository) :
			Material(PHONG_MATERIAL_NAME, context, texture_repository) {
			resource_manager = std::make_shared<MaterialResourceManager<PhongResources>>(vulkan_context);
		}

		void buildPipelines(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) override;
		void writeMaterial() override;
		std::shared_ptr<MaterialInstance> createInstance(const PhongInstance::Parameters &parameters
		);
		void addInstanceToResources(MaterialInstance &inst) override;
		void addInstanceToResources(PhongInstance &inst);

		[[nodiscard]] std::vector<std::shared_ptr<PhongResources>> getResources() const;
		std::vector<std::shared_ptr<Texture>> getTextures() override;
		void* getPushConstants(uint32_t *out_size) override;
		void reset() override;

	protected:
		void initProperties() override;

		[[nodiscard]] std::shared_ptr<PhongResources> mapInstanceToResources(const PhongInstance &instance) const;

		VkDescriptorSetLayout createLayout() override;
		std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) override;

	private:
		std::shared_ptr<MaterialResourceManager<PhongResources>> resource_manager;
		PushConstants push_constants{};
	};

} // namespace RtEngine
#endif // PHONGMATERIAL_HPP
