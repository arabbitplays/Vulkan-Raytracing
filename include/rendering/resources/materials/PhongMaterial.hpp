#ifndef PHONGMATERIAL_HPP
#define PHONGMATERIAL_HPP
#include <Material.hpp>
#include <glm/vec3.hpp>

#define PHONG_MATERIAL_NAME "phong"

namespace RtEngine {
class PhongMaterial : public Material {
public:
    struct PhongMaterialConstants {
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 ambient;
        glm::vec3 reflection;
        glm::vec3 transmission;
        float n;
        glm::vec4 eta; // only xyz for the eta of each rgb channel
    };

    struct MaterialResources {
        std::shared_ptr<PhongMaterialConstants> constants;
        // add images and samplers here
    };

    struct MaterialProperties {
        int32_t shadows, dispersion, fresnel;
    };

    PhongMaterial(std::shared_ptr<VulkanContext> context, std::shared_ptr<RuntimeContext> runtime_context) : Material(PHONG_MATERIAL_NAME, context, runtime_context) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    void writeMaterial() override;
    std::shared_ptr<MaterialInstance> createInstance(glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient, glm::vec3 reflection, glm::vec3 transmission, float n, glm::vec3 eta = glm::vec3(0.0));
    std::vector<std::shared_ptr<MaterialResources>> getResources();
    std::vector<std::shared_ptr<Texture>> getTextures() override;
    void reset() override;

protected:
    void initProperties() override;
private:
    AllocatedBuffer createMaterialBuffer();

    MaterialProperties material_properties;
    std::vector<std::shared_ptr<MaterialResources>> resources_buffer;
    AllocatedBuffer materialBuffer;
};



}
#endif //PHONGMATERIAL_HPP
