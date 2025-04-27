#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <DescriptorAllocator.hpp>
#include <vulkan/vulkan_core.h>
#include <RessourceBuilder.hpp>
#include <bits/shared_ptr.h>
#include <glm/vec4.hpp>
#include <Pipeline.hpp>
#include <PropertiesManager.hpp>

namespace RtEngine {
struct MaterialInstance;
class Pipeline;

class Material {
  public:
    Material() = default;
    Material(std::string name, std::shared_ptr<VulkanContext> context) : name(name), context(context) {
        std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
        };
        descriptorAllocator.init(context->device_manager->getDevice(), 4, poolRatios);

        mainDeletionQueue.pushFunction([&]() {
            descriptorAllocator.destroyPools(this->context->device_manager->getDevice());
        });
    };

    virtual ~Material() {};

    std::shared_ptr<Pipeline> pipeline;
    VkDescriptorSetLayout materialLayout;
    VkDescriptorSet materialDescriptorSet;

    virtual void buildPipelines(VkDescriptorSetLayout sceneLayout) = 0;
    virtual void writeMaterial() = 0;
    virtual glm::vec4 getEmissionForInstance(uint32_t material_instance_id) {
        return glm::vec4(0.0f);
    }
    std::vector<std::shared_ptr<MaterialInstance>> getInstances();
    std::shared_ptr<Properties> getProperties();

    void clearRessources();
    virtual void reset();

    std::string name;

protected:
    virtual void initProperties() = 0;

    std::shared_ptr<VulkanContext> context;
    DescriptorAllocator descriptorAllocator;
    DeletionQueue mainDeletionQueue, resetQueue;

    std::vector<std::shared_ptr<MaterialInstance>> instances;
    std::shared_ptr<Properties> properties;

    AllocatedBuffer material_buffer; // maps an instance to its respective material via a common index into the constants and texture buffers
};

struct MaterialInstance {
    std::shared_ptr<Properties> properties;
    uint32_t material_index;
    uint32_t albedo_texture_index;
};

}
#endif //MATERIAL_HPP
