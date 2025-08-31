//
// Created by oschdi on 18.08.25.
//

#ifndef ENGINERESOURCES_H
#define ENGINERESOURCES_H
#include "ILayoutProvider.hpp"
#include "VulkanContext.hpp"


namespace RtEngine {
    struct SvgfData {
        glm::vec3 color;
        glm::vec3 position;
        glm::vec3 normal;
        float depth;
        glm::vec2 motion;
    };

    struct SvgfHistData {
        glm::vec3 color;
        glm::vec3 normal;
        float depth;
        float padding;
    };

    class EngineResources : public ILayoutProvider {
    public:
        EngineResources() = default;
        EngineResources(const std::shared_ptr<VulkanContext> &vulkanContext);

        void createAndBindResources() override;
        void destroyResources() override;

    private:
        VkDescriptorSetLayout createLayout() override;
        std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) override;

        void createEngineBuffers();
        void bindEngineBuffers(const VkDescriptorSet &descriptor_set) const;

        std::shared_ptr<VulkanContext> vulkan_context;

        AllocatedBuffer denoisingBuffer, denoisingHistBuffer;

    };
}


#endif //ENGINERESOURCES_H
