#ifndef PHONGMATERIAL_HPP
#define PHONGMATERIAL_HPP
#include <Material.hpp>

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

		struct MaterialProperties {
			int32_t shadows, dispersion, fresnel;
		};

		PhongMaterial(const std::shared_ptr<VulkanContext> &context, const std::shared_ptr<TextureRepository> &texture_repository) :
			Material(PHONG_MATERIAL_NAME, context, texture_repository) {
			resource_manager = std::make_shared<MaterialResourceManager<PhongResources>>(vulkan_context);
		}

		void buildPipelines() override;
		void writeMaterial() override;
		std::shared_ptr<MaterialInstance> createInstance(glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient,
														 glm::vec3 reflection, glm::vec3 transmission, float n,
														 glm::vec3 eta = glm::vec3(0.0));
		std::vector<std::shared_ptr<PhongResources>> getResources() const;
		std::vector<std::shared_ptr<Texture>> getTextures() override;
		void reset() override;

	protected:
		void initProperties() override;

		VkDescriptorSetLayout createLayout() override;
		std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) override;

	private:
		std::shared_ptr<MaterialResourceManager<PhongResources>> resource_manager;
		MaterialProperties material_properties{};
	};

} // namespace RtEngine
#endif // PHONGMATERIAL_HPP
