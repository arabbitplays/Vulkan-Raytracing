//
// Created by oschdi on 2/26/25.
//

#include "BenchmarkRenderer.hpp"

void BenchmarkRenderer::mainLoop() {
    assert(!renderer_options->output_dir.empty());
    assert(!renderer_options->reference_scene_path.empty());

    loadScene();
    int texWidth, texHeight, texChannels;
    reference_image_data = ressourceBuilder.loadImageData(renderer_options->reference_image_path, &texWidth, &texHeight, &texChannels);

    while(!glfwWindowShouldClose(window)) {
        // render one image and then output it if output path is defined
        if (renderer_options->sample_count == raytracing_options->curr_sample_count)
        {
            vkDeviceWaitIdle(device);
            outputRenderingTarget();
            break;
        }

        glfwPollEvents();

        drawFrame();

        if (calculate_error)
        {
            calculate_error = false;
            calculateMSEToReference();
        }
    }

    vkDeviceWaitIdle(device);
}

void BenchmarkRenderer::loadScene()
{
    vkDeviceWaitIdle(device);
    raytracing_options->curr_sample_count = 0;
    std::string path = renderer_options->reference_scene_path;
    scene_manager->createScene(path);
}


void BenchmarkRenderer::drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t curr_sample_count = raytracing_options->curr_sample_count;
    if (error_calculation_sample_count == curr_sample_count)
    {
        present_image = true;
        calculate_error = true;
        error_calculation_sample_count = (error_calculation_count + 2) * (error_calculation_count + 2);
        error_calculation_count++;
    }

    int  imageIndex = 0;
    if (present_image)
    {
        imageIndex = aquireNextSwapchainImage();
        if (imageIndex < 0)
            return;
    }

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    scene_manager->updateScene(mainDrawContext, currentFrame, getRenderTarget(), rng_tex);
    raytracing_options->emitting_instances_count = scene_manager->getEmittingInstancesCount(); // TODO move this together with the creation of the instance buffers

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    if (present_image)
    {
        std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[currentFrame]};
        std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[currentFrame]};
        submitCommandBuffer(waitSemaphore, signalSemaphore);
        presentSwapchainImage(signalSemaphore, imageIndex);
    } else
    {
        submitCommandBuffer({}, {});
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void BenchmarkRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    recordBeginCommandBuffer(commandBuffer);
    recordRenderToImage(commandBuffer);
    if (present_image)
        recordCopyToSwapchain(commandBuffer, imageIndex);
    recordEndCommandBuffer(commandBuffer);
}

float BenchmarkRenderer::calculateMSEToReference()
{
    void* data = context->resource_builder->downloadImage(render_targets[0]);
}


void BenchmarkRenderer::outputRenderingTarget()
{
    void* data = context->resource_builder->downloadImage(render_targets[0]);
    fixImageFormatForStorage(static_cast<unsigned char*>(data), render_targets[0].imageExtent.width * render_targets[0].imageExtent.height, render_targets[0].imageFormat);
    context->resource_builder->writePNG(renderer_options->output_dir + "/" + std::to_string(renderer_options->sample_count) + "_ref.png", data, render_targets[0].imageExtent.width, render_targets[0].imageExtent.height);
}

// target format is R8G8B8A8
void BenchmarkRenderer::fixImageFormatForStorage(unsigned char* image_data, size_t pixel_count, VkFormat originalFormat)
{
    if (originalFormat == VK_FORMAT_R8G8B8A8_UNORM)
        return;

    if (originalFormat == VK_FORMAT_B8G8R8A8_UNORM)
    {
        for (size_t i = 0; i < pixel_count; i++) {
            std::swap(image_data[i * 4], image_data[i * 4 + 2]);  // Swap B (0) and R (2)
        }
    } else
    {
        spdlog::error("Image format of the storage image is not supported to be stored correctly!");
    }
}