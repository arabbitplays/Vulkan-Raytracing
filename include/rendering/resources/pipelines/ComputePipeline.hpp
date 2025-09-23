//
// Created by oschdi on 23.09.25.
//

#ifndef VULKAN_RAYTRACING_COMPUTEPIPELINE_HPP
#define VULKAN_RAYTRACING_COMPUTEPIPELINE_HPP
#include "Pipeline.hpp"

namespace RtEngine {
    class ComputePipeline : public Pipeline {
    public:
        explicit ComputePipeline(const std::shared_ptr<VulkanContext> &context) : Pipeline(context) { }

        void build() override;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTEPIPELINE_HPP