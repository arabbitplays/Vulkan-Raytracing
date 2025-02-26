//
// Created by oschdi on 2/26/25.
//

#ifndef BENCHMARKRENDERER_HPP
#define BENCHMARKRENDERER_HPP
#include <VulkanEngine.hpp>


class BenchmarkRenderer : public VulkanEngine {
    void mainLoop() override;
    void loadScene() override;
    void drawFrame() override;
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;

    float calculateMSEToReference();

private:
    uint8_t* reference_image_data;
    int ref_width, ref_height, ref_channels;

    uint32_t error_calculation_sample_count = 1;
    bool present_image = false, calculate_error = true;
};



#endif //BENCHMARKRENDERER_HPP
