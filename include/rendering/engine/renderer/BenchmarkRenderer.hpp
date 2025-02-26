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
    void outputRenderingTarget();
    void fixImageFormatForStorage(unsigned char* image_data, size_t pixel_count, VkFormat originalFormat);

private:
    stbi_uc* reference_image_data;

    uint32_t error_calculation_count = 0;
    uint32_t error_calculation_sample_count = 1;
    bool present_image = false, calculate_error = true;
};



#endif //BENCHMARKRENDERER_HPP
