//
// Created by oschdi on 23.09.25.
//

#ifndef VULKAN_RAYTRACING_SVGFDENOISER_HPP
#define VULKAN_RAYTRACING_SVGFDENOISER_HPP
#include <memory>

#include "ComputePipeline.hpp"
#include "GBuffer.hpp"

namespace RtEngine {
    class SvgfDenoiser : public ILayoutProvider {
    public:
        explicit SvgfDenoiser(const std::shared_ptr<VulkanContext> &context);

        void createComputePipeline();
        void writeResources(const AllocatedImage &target_image, const std::shared_ptr<GBuffer> &g_buffer) const;
        void recordCommands(VkCommandBuffer command_buffer);

        void destroyLayout() override;

        void destroyResources();

    private:
        VkDescriptorSetLayout createLayout() override;
        std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) override;

        std::shared_ptr<VulkanContext> context;
        DeletionQueue deletion_queue;
        std::shared_ptr<ComputePipeline> compute_pipeline;
    };
}



#endif //VULKAN_RAYTRACING_SVGFDENOISER_HPP