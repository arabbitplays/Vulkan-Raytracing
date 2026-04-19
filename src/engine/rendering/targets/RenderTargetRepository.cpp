#include "../../../../include/engine/rendering/targets/RenderTargetRepository.hpp"

#include <assert.h>
#include <RandomUtil.hpp>

#include "targets/RaytracingTarget.hpp"
#include "targets/RngTarget.hpp"

namespace RtEngine {
    RenderTargetRepository::RenderTargetRepository(const std::shared_ptr<ResourceBuilder> &resource_builder,
                                                   VkExtent2D image_extent, uint32_t max_frames_in_flight)
        : resource_builder(resource_builder), max_frames_in_flight(max_frames_in_flight), image_extent(image_extent) {
        init();
    }

    void RenderTargetRepository::addRenderTarget(std::string key, RenderTargetType type) {
        std::shared_ptr<RenderTarget> render_target;
        switch (type) {
            case RAYTRACE:
                render_target = std::make_shared<RaytracingTarget>(resource_builder, max_frames_in_flight);
                break;
            case RNG:
                render_target = std::make_shared<RngTarget>(resource_builder, max_frames_in_flight);
                break;
        }
        render_target->createTargetImages(image_extent);
        render_targets[key] = render_target;
    };

    void RenderTargetRepository::init() {
        addRenderTarget("main", RAYTRACE);
        addRenderTarget("rng", RNG);
        addRenderTarget("diff", RAYTRACE);
    }

    void RenderTargetRepository::recreate(const VkExtent2D new_image_extent) {
        this->image_extent = new_image_extent;
        for (const auto &render_target : render_targets) {
            render_target.second->recreate(new_image_extent);
        }
        resetAccumulatedFrames();
    }

    AllocatedImage RenderTargetRepository::getLastTargetImage() const {
        uint32_t idx = current_image_idx != 0 ? current_image_idx - 1 : max_frames_in_flight - 1;
        return render_targets.at("main")->getImage(idx);
    }

    AllocatedImage RenderTargetRepository::getCurrRenderTargetImage(std::string key) const {
        assert(render_targets.contains(key));
        return render_targets.at(key)->getImage(current_image_idx);
    }

    void RenderTargetRepository::nextImage() {
        current_image_idx = (current_image_idx + 1) % max_frames_in_flight;
    }

    VkExtent2D RenderTargetRepository::getExtent() const {
        return image_extent;
    }

    uint32_t RenderTargetRepository::getAccumulatedFrameCount() const {
        return accumulated_frame_count;
    }

    void RenderTargetRepository::resetAccumulatedFrames() {
        accumulated_frame_count = 0;
    }

    void RenderTargetRepository::incrementAccumulatedFrameCount() {
        accumulated_frame_count++;
    }

    uint32_t RenderTargetRepository::getTotalSampleCount() const {
        return accumulated_frame_count * samples_per_frame;
    }

    uint32_t RenderTargetRepository::getSamplesPerFrame() const {
        return samples_per_frame;
    }

    uint32_t RenderTargetRepository::getDiffSamplesPerFrame() const {
        return diff_samples_per_frame;
    }

    void RenderTargetRepository::setSamplesPerFrame(uint32_t new_samples_per_frame,
                                                    uint32_t new_diff_samples_per_frame) {
        samples_per_frame = new_samples_per_frame;
        diff_samples_per_frame = new_diff_samples_per_frame;
    }

    void RenderTargetRepository::destroy() {
        for (const auto &render_target : render_targets) {
            render_target.second->destroy();
        }
        render_targets.clear();
    }
}
