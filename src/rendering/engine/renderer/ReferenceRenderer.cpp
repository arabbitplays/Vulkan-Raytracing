//
// Created by oschdi on 2/16/25.
//

#include "ReferenceRenderer.hpp"

void ReferenceRenderer::mainLoop() {
    assert(!renderer_options->output_dir.empty());
    assert(!renderer_options->reference_scene_path.empty());

    loadScene();

    stopwatch.reset();

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
    }

    vkDeviceWaitIdle(device);
}

void ReferenceRenderer::loadScene()
{
    vkDeviceWaitIdle(device);
    raytracing_options->curr_sample_count = 0;
    std::string path = renderer_options->reference_scene_path;
    scene_manager->createScene(path);
}


void ReferenceRenderer::drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t curr_sample_count = raytracing_options->curr_sample_count;
    present_image = present_sample_count == curr_sample_count;

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

        present_sample_count *= 2;
    } else
    {
        submitCommandBuffer({}, {});
    }

    if (curr_sample_count % 1000 == 0)
    {
        double elapsed_time = stopwatch.elapsed().count();
        stopwatch.reset();
        uint32_t samples_left = renderer_options->sample_count - curr_sample_count;
        double time_left = elapsed_time / 1000 * samples_left;
        int hours = static_cast<int>(time_left) / 3600;
        int minutes = (static_cast<int>(time_left) % 3600) / 60;
        int sec = static_cast<int>(time_left) % 60;
        uint32_t progress = round((float)curr_sample_count / (float)renderer_options->sample_count * 100);
        spdlog::info("Current sample count: {}, progress: {}%, estimated time remaining: {}h {}m {}s", curr_sample_count, progress, hours, minutes, sec);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void ReferenceRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    recordBeginCommandBuffer(commandBuffer);
    recordRenderToImage(commandBuffer);
    if (present_image)
        recordCopyToSwapchain(commandBuffer, imageIndex);
    recordEndCommandBuffer(commandBuffer);
}

void ReferenceRenderer::outputRenderingTarget()
{
    void* data = context->resource_builder->downloadImage(render_targets[0], sizeof(uint32_t));
    uint8_t* fixed_data = fixImageFormatForStorage(data, render_targets[0].imageExtent.width * render_targets[0].imageExtent.height, render_targets[0].imageFormat);
    context->resource_builder->writePNG(renderer_options->output_dir + "/" + std::to_string(renderer_options->sample_count) + "_ref.png", fixed_data, render_targets[0].imageExtent.width, render_targets[0].imageExtent.height);
}

// target format is R8G8B8A8_UNORM
uint8_t* ReferenceRenderer::fixImageFormatForStorage(void* data, size_t pixel_count, VkFormat originalFormat)
{
    if (originalFormat == VK_FORMAT_R8G8B8A8_UNORM)
        return static_cast<uint8_t*>(data);

    if (originalFormat == VK_FORMAT_B8G8R8A8_UNORM)
    {
        auto image_data = static_cast<uint8_t*>(data);
        for (size_t i = 0; i < pixel_count; i++) {
            std::swap(image_data[i * 4], image_data[i * 4 + 2]);  // Swap B (0) and R (2)
        }
        return image_data;
    } if (originalFormat == VK_FORMAT_R32G32B32A32_SFLOAT)
    {
        uint8_t* output_image = new uint8_t[pixel_count * 4];
        auto image_data = static_cast<float*>(data);
        for (size_t i = 0; i < pixel_count * 4; i++) {
            float test = image_data[i];
            // Clamp each channel to the [0, 1] range and then scale to [0, 255]
            output_image[i] = static_cast<uint8_t>(std::fmin(1.0f, std::fmax(0.0f, image_data[i])) * 255);
        }
        return output_image;
    } else
    {
        spdlog::error("Image format of the storage image is not supported to be stored correctly!");
    }
}
