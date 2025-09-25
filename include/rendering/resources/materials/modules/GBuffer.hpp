//
// Created by oschdi on 25.09.25.
//

#ifndef VULKAN_RAYTRACING_GBUFFER_HPP
#define VULKAN_RAYTRACING_GBUFFER_HPP

#include <glm/vec3.hpp>

#include "ResourceBuilder.hpp"

namespace RtEngine {
    struct GBufferData {
        glm::vec3 color;
        glm::vec3 position;
        glm::vec3 normal;
        float depth;
        glm::vec2 motion;
        int32_t instance_id;
        glm::vec3 padding;
    };

    struct GBufferHistData {
        glm::vec3 color;
        glm::vec3 normal;
        float depth;
        float instance_id;
    };

    class GBuffer {
    public:
        GBuffer(const std::shared_ptr<ResourceBuilder>& resource_builder) : resource_builder(resource_builder) { }

        void create(const uint pixel_count) {
            destroy();

            curr_pixel_count = pixel_count;
            g_buffer = resource_builder->createZeroBuffer(pixel_count * sizeof(GBufferData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            hist_g_buffer = resource_builder->createZeroBuffer(pixel_count * sizeof(GBufferHistData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }

        void destroy() const {
            if (g_buffer.handle != VK_NULL_HANDLE) {
                resource_builder->destroyBuffer(g_buffer);
                resource_builder->destroyBuffer(hist_g_buffer);
            }
        }

        AllocatedBuffer getBuffer() const {
            return g_buffer;
        }

        uint32_t getBufferSize() const {
            return curr_pixel_count * sizeof(GBufferData);
        }

        AllocatedBuffer getHistBuffer() const {
            return  hist_g_buffer;
        }

        uint32_t getHistBufferSize() const {
            return curr_pixel_count * sizeof(GBufferHistData);
        }

        uint32_t getCurrPixelCount() const {
            return curr_pixel_count;
        }

    private:
        std::shared_ptr<ResourceBuilder> resource_builder;

        uint32_t curr_pixel_count = 0;
        AllocatedBuffer g_buffer, hist_g_buffer;
    };
}

#endif //VULKAN_RAYTRACING_GBUFFER_HPP