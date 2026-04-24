#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP
#include <memory>
#include <ResourceBuilder.hpp>
#include <Texture.hpp>
#include <vector>

#include "RenderTarget.hpp"
#include "RenderTargetType.hpp"

namespace RtEngine
{


    class RenderTargetRepository {
    public:
        RenderTargetRepository() = default;
        explicit RenderTargetRepository(const std::shared_ptr<ResourceBuilder>& resource_builder, VkExtent2D image_extent, uint32_t max_frames_in_flight);

        void addRenderTarget(std::string key, RenderTargetType type);

        AllocatedImage getCurrRenderTargetImage(std::string key) const;
        AllocatedImage getLastRenderTargetImage(std::string key) const;

        void nextImage();

        VkExtent2D getExtent() const;

        uint32_t getAccumulatedFrameCount() const;
        uint32_t getAccumulatedDiffFrameCount() const;

        void resetAccumulatedFrames();
        void incrementAccumulatedFrameCount();
        void incrementAccumulatedDiffFrameCount();

        uint32_t getTotalSampleCount() const;

        uint32_t getSamplesPerFrame() const;
        uint32_t getDiffSamplesPerFrame() const;
        void setSamplesPerFrame(uint32_t new_samples_per_frame, uint32_t new_diff_samples_per_frame);

        void destroy();

        void recreate(VkExtent2D new_image_extent);

    private:
        void init();

        std::shared_ptr<ResourceBuilder> resource_builder;

        uint32_t max_frames_in_flight;
        VkExtent2D image_extent;

        uint32_t current_image_idx = 0;

        std::unordered_map<std::string, std::shared_ptr<RenderTarget>> render_targets{};

        uint32_t accumulated_frame_count = 0;
        uint32_t accumulated_diff_frame_count = 0;
        uint32_t samples_per_frame = 8;
        uint32_t diff_samples_per_frame = 1;
    };
}



#endif //RENDERTARGET_HPP
