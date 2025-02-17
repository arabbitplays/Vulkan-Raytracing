//
// Created by oschdi on 2/16/25.
//

#include "ReferenceRenderer.hpp"

void ReferenceRenderer::mainLoop() {
    assert(!renderer_options->output_path.empty());
    assert(!renderer_options->reference_scene_path.empty());

    renderer_options->curr_scene_path = renderer_options->reference_scene_path;
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

