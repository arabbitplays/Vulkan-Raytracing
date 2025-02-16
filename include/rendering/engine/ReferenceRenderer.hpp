//
// Created by oschdi on 2/16/25.
//

#ifndef REFERENCERENDERER_HPP
#define REFERENCERENDERER_HPP

#include <VulkanEngine.hpp>
#include "spdlog/stopwatch.h"

class ReferenceRenderer : public VulkanEngine {
    void mainLoop() override;
    void drawFrame() override;
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;

private:
    spdlog::stopwatch stopwatch;
    uint32_t present_sample_count = 1;
    bool present_image = false;
};



#endif //REFERENCERENDERER_HPP
