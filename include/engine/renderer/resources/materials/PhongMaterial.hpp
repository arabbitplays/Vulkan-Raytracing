#ifndef PHONGMATERIAL_HPP
#define PHONGMATERIAL_HPP
#include <Material.hpp>
#include <glm/vec3.hpp>

#define PHONG_MATERIAL_NAME "phong"

namespace RtEngine {
	class PhongMaterial : public Material {
	public:


		struct MaterialProperties {
			int32_t shadows, dispersion, fresnel;
		};

		PhongMaterial(std::shared_ptr<VulkanContext> context, std::shared_ptr<TextureRepository> tex_repo) :
			Material(PHONG_MATERIAL_NAME, context, tex_repo) {}

		void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
		void writeMaterial(AllocatedBuffer material_buffer, std::shared_ptr<MaterialTextures<>> material_textures) override;
		std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node) override;

		void reset() override;

	protected:
		void initProperties() override;

	private:

		MaterialProperties material_properties;
	};

} // namespace RtEngine
#endif // PHONGMATERIAL_HPP
