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

		PhongMaterial(std::shared_ptr<VulkanContext> context, std::shared_ptr<RuntimeContext> runtime_context) :
			Material(PHONG_MATERIAL_NAME, context, runtime_context) {}

		void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
		void writeMaterial() override;
		std::shared_ptr<MaterialInstance> loadInstance(const YAML::Node &yaml_node);

		std::vector<std::shared_ptr<Texture>> getTextures() override;
		void reset() override;

	protected:
		void initProperties() override;

	private:
		AllocatedBuffer createMaterialBuffer();

		MaterialProperties material_properties;
	};

} // namespace RtEngine
#endif // PHONGMATERIAL_HPP
