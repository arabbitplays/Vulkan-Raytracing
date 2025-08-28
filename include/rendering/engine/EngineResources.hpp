//
// Created by oschdi on 18.08.25.
//

#ifndef ENGINERESOURCES_H
#define ENGINERESOURCES_H
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

    class EngineResources {
    public:
        EngineResources() = default;
        EngineResources(const std::shared_ptr<VulkanContext> &vulkanContext);

        void createEngineLayout();
        void createEngineDescriptorSet(const VkDescriptorSetLayout &layout);
        void createAndBindEngineBuffers();

        VkDescriptorSetLayout getEngineLayout() const;
        VkDescriptorSet getEngineDescriptorSet() const;

        void destroy();

    private:
        void createEngineBuffers();
        void bindEngineBuffers(const VkDescriptorSet &descriptor_set);

        std::shared_ptr<VulkanContext> vulkan_context;
        DeletionQueue main_deletion_queue;

        VkDescriptorSetLayout engine_descriptor_set_layout;
        VkDescriptorSet engine_descriptor_set;

        AllocatedBuffer denoisingBuffer, denoisingHistBuffer;

    };
}


#endif //ENGINERESOURCES_H
