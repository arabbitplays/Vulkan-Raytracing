#ifndef VULKAN_RAYTRACING_SAMPLECOUNTTARGET_HPP
#define VULKAN_RAYTRACING_SAMPLECOUNTTARGET_HPP
#include "RenderTarget.hpp"

namespace RtEngine {
    class SampleCountTarget : public RenderTarget {
    public:
        SampleCountTarget(const std::shared_ptr<ResourceBuilder> &resource_builder, uint32_t max_frames_in_flight)
            : RenderTarget(resource_builder, max_frames_in_flight) {
        }

        void createTargetImages(VkExtent2D image_extent) override;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_SAMPLECOUNTTARGET_HPP
