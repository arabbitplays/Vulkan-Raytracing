//
// Created by oschdi on 23.09.25.
//

#ifndef VULKAN_RAYTRACING_SVGFDENOISER_HPP
#define VULKAN_RAYTRACING_SVGFDENOISER_HPP
#include <memory>

#include "ComputePipeline.hpp"

namespace RtEngine {
    class SvgfDenoiser {
    public:
        explicit SvgfDenoiser(const std::shared_ptr<VulkanContext> &context);

        void createComputePipeline(std::vector<VkDescriptorSetLayout> layouts);
        void recordCommands();
    private:
        std::shared_ptr<VulkanContext> context;
        std::shared_ptr<ComputePipeline> compute_pipeline;
    };
}



#endif //VULKAN_RAYTRACING_SVGFDENOISER_HPP