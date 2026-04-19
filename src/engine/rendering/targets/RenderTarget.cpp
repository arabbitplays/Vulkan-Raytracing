#include "targets/RenderTarget.hpp"

#include <cassert>

namespace RtEngine {
    RenderTarget::RenderTarget(const std::shared_ptr<ResourceBuilder> &resource_builder,
                               uint32_t max_frames_in_flight) : resource_builder(resource_builder),
                                                                max_frames_in_flight(max_frames_in_flight) {
    }

    void RenderTarget::recreate(VkExtent2D image_extent) {
        destroy();
        createTargetImages(image_extent);
    }

    AllocatedImage RenderTarget::getImage(uint32_t idx) {
        assert(idx < max_frames_in_flight && !target_images.empty());
        return target_images[idx];
    }

    void RenderTarget::destroy() {
        for (const auto &target : target_images) {
            resource_builder->destroyImage(target);
        }
        target_images.clear();
    }
} // RtEngine
