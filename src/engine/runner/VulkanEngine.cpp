#include <HierarchyWindow.hpp>
#include <SceneWriter.hpp>
#include <VulkanEngine.hpp>
#include <cstdlib>
#include <filesystem>
#include <PathUtil.hpp>
#include <set>
#include <RandomUtil.hpp>

namespace RtEngine {

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void CmdTraceRaysKHR(VkDevice device, VkCommandBuffer commandBuffer,
						 const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
						 const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
						 const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
						 const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width,
						 uint32_t height, uint32_t depth) {
		auto func = (PFN_vkCmdTraceRaysKHR) vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
		if (func != nullptr) {
			return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable,
						pCallableShaderBindingTable, width, height, depth);
		}
	}

	const uint32_t WIDTH = 6144;
	const uint32_t HEIGHT = 3320;

	void VulkanEngine::run(const std::string &config_file, const std::string &resources_dir) {
		vulkan_context = std::make_shared<VulkanContext>();
		vulkan_context->base_options = std::make_shared<BaseOptions>();
		vulkan_context->base_options->resources_dir = resources_dir;
		properties_manager = std::make_shared<PropertiesManager>(config_file);

		initWindow();
		initVulkan();
		initGui();

		mainLoop();

		cleanup();
	}

	void VulkanEngine::initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHintString(GLFW_WAYLAND_APP_ID, "test");

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		glfwSetKeyCallback(window, keyCallback);
		glfwSetCursorPosCallback(window, mouseCallback);
	}

	void VulkanEngine::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
		auto app = reinterpret_cast<VulkanEngine *>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void VulkanEngine::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		auto app = reinterpret_cast<VulkanEngine *>(glfwGetWindowUserPointer(window));
		if (app->scene_manager->scene != nullptr && app->scene_manager->scene->camera != nullptr) {
			app->scene_manager->scene->camera->processGlfwKeyEvent(key, action);
		}
	}

	void VulkanEngine::mouseCallback(GLFWwindow *window, double xPos, double yPos) {
		auto app = reinterpret_cast<VulkanEngine *>(glfwGetWindowUserPointer(window));
		if (app->scene_manager->scene != nullptr && app->scene_manager->scene->camera != nullptr) {
			app->scene_manager->scene->camera->processGlfwMouseEvent(xPos, yPos);
		}
	}

	void VulkanEngine::initGui() {
		guiManager = std::make_shared<GuiManager>(vulkan_context);

		mainDeletionQueue.pushFunction([&]() { guiManager->destroy(); });

		auto options_window = std::make_shared<OptionsWindow>(properties_manager);
		options_window->addCallback([this](uint32_t flags) {
			this->handleGuiUpdate(flags);
		});
		guiManager->addWindow(options_window);

		auto inspector_window = std::make_shared<InspectorWindow>(scene_manager);
		inspector_window->addCallback([this](uint32_t flags) {
			this->handleGuiUpdate(flags);
		});
		guiManager->addWindow(inspector_window);

		auto hierarchy_window = std::make_shared<HierarchyWindow>(inspector_window, scene_manager);
		hierarchy_window->addCallback([this](uint32_t flags) {
			this->handleGuiUpdate(flags);
		});
		guiManager->addWindow(hierarchy_window);
	}

	void VulkanEngine::initVulkan() {
		createVulkanContext();
		createRuntimeContext();

		mainDrawContext = std::make_shared<DrawContext>();
		mainDrawContext->max_frames_in_flight = max_frames_in_flight;
		mainDrawContext->target = std::make_shared<RenderTarget>(vulkan_context->resource_builder, vulkan_context->swapchain->extent, mainDrawContext->max_frames_in_flight);
		mainDeletionQueue.pushFunction([&]() { mainDrawContext->target->destroy(); });

		scene_manager = std::make_shared<SceneManager>(vulkan_context, runtime_context, mainDrawContext->max_frames_in_flight,
													   DeviceManager::RAYTRACING_PROPERTIES);
		mainDeletionQueue.pushFunction([&]() { scene_manager->clearResources(); });

		createCommandBuffers();
		createSyncObjects();
	}

	void VulkanEngine::createVulkanContext() {
		vulkan_context->window = window;
		vulkan_context->device_manager = std::make_shared<DeviceManager>(window, enableValidationLayers);

		initProperties();
		properties_manager->addPropertySection(renderer_properties);

		vulkan_context->command_manager = std::make_shared<CommandManager>(vulkan_context->device_manager);
		vulkan_context->resource_builder =
				std::make_shared<ResourceBuilder>(vulkan_context->device_manager, vulkan_context->command_manager,
												  vulkan_context->base_options->resources_dir);
		vulkan_context->swapchain =
				std::make_shared<Swapchain>(vulkan_context->device_manager, window, vulkan_context->resource_builder);
		vulkan_context->descriptor_allocator = createDescriptorAllocator();

		mainDeletionQueue.pushFunction([&]() {
			vulkan_context->descriptor_allocator->destroyPools(vulkan_context->device_manager->getDevice());
			vulkan_context->swapchain->destroy();
			vulkan_context->command_manager->destroyCommandManager();
			vulkan_context->device_manager->destroy();
		});
	}

	void VulkanEngine::createRuntimeContext() {
		runtime_context = std::make_shared<RuntimeContext>();
		runtime_context->mesh_repository = std::make_shared<MeshRepository>(vulkan_context);
		runtime_context->texture_repository = std::make_shared<TextureRepository>(vulkan_context->resource_builder);

		mainDeletionQueue.pushFunction([&]() {
			runtime_context->mesh_repository->destroy();
			runtime_context->texture_repository->destroy();
		});
	}

	std::shared_ptr<DescriptorAllocator> VulkanEngine::createDescriptorAllocator() {
		std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
				{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
		};

		auto descriptorAllocator = std::make_shared<DescriptorAllocator>();
		descriptorAllocator->init(vulkan_context->device_manager->getDevice(), 8, poolRatios);

		return descriptorAllocator;
	}

	bool VulkanEngine::hasStencilComponent(const VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void VulkanEngine::loadScene() {
		assert(!vulkan_context->base_options->curr_scene_name.empty());
		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice()); // TODO is this needed?
		mainDrawContext->target->resetAccumulatedFrames();
		std::string path = vulkan_context->base_options->resources_dir + "/scenes/" +
						   vulkan_context->base_options->curr_scene_name;
		scene_manager->createScene(path);
		properties_manager->addPropertySection(scene_manager->scene->material->getProperties());

		SceneWriter writer;
		writer.writeScene(PathUtil::getFileName(scene_manager->getSceneInformation().path), scene_manager->scene);
	}

	void VulkanEngine::createCommandBuffers() {
		commandBuffers.resize(mainDrawContext->max_frames_in_flight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = vulkan_context->command_manager->commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

		if (vkAllocateCommandBuffers(vulkan_context->device_manager->getDevice(), &allocInfo, commandBuffers.data()) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command bufer!");
		}
	}

	void VulkanEngine::createSyncObjects() {
		imageAvailableSemaphores.resize(mainDrawContext->max_frames_in_flight);
		renderFinishedSemaphores.resize(mainDrawContext->max_frames_in_flight);
		inFlightFences.resize(mainDrawContext->max_frames_in_flight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < mainDrawContext->max_frames_in_flight; i++) {
			if (vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(vulkan_context->device_manager->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) !=
						VK_SUCCESS) {
				throw std::runtime_error("failed to create sync objects");
			}

			mainDeletionQueue.pushFunction([&, i]() {
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), imageAvailableSemaphores[i], nullptr);
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), renderFinishedSemaphores[i], nullptr);
				vkDestroyFence(vulkan_context->device_manager->getDevice(), inFlightFences[i], nullptr);
			});
		}
	}

	void VulkanEngine::mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			if (PathUtil::getFile(scene_manager->getSceneInformation().path) != vulkan_context->base_options->curr_scene_name) {
				loadScene();
			}
			scene_manager->updateScene(mainDrawContext);
			drawFrame();
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}

	void VulkanEngine::drawFrame() {
		vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame], VK_TRUE,
						UINT64_MAX);

		const int32_t imageIndex = aquireNextSwapchainImage();
		if (imageIndex < 0)
			return;

		vkResetFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame]);

		vkResetCommandBuffer(commandBuffers[mainDrawContext->currentFrame], 0);
		recordCommandBuffer(commandBuffers[mainDrawContext->currentFrame], imageIndex);

		std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[mainDrawContext->currentFrame]};
		std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[mainDrawContext->currentFrame]};
		submitCommandBuffer(waitSemaphore, signalSemaphore);
		presentSwapchainImage(signalSemaphore, imageIndex);

		mainDrawContext->nextFrame();
	}

	int32_t VulkanEngine::aquireNextSwapchainImage() {
		uint32_t imageIndex;
		VkResult result =
				vkAcquireNextImageKHR(vulkan_context->device_manager->getDevice(), vulkan_context->swapchain->handle,
									  UINT64_MAX, imageAvailableSemaphores[mainDrawContext->currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			refreshAfterResize();
			return -1;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		return static_cast<int32_t>(imageIndex);
	}

	void VulkanEngine::submitCommandBuffer(std::vector<VkSemaphore> wait_semaphore,
										   std::vector<VkSemaphore> signal_semaphore) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
		submitInfo.pWaitSemaphores = wait_semaphore.data();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[mainDrawContext->currentFrame];
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphore.size());
		;
		submitInfo.pSignalSemaphores = signal_semaphore.data();

		if (vkQueueSubmit(vulkan_context->device_manager->getQueue(GRAPHICS), 1, &submitInfo,
						  inFlightFences[mainDrawContext->currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}

	void VulkanEngine::presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, const uint32_t image_index) {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
		presentInfo.pWaitSemaphores = wait_semaphore.data();
		VkSwapchainKHR swapChains[] = {vulkan_context->swapchain->handle};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &image_index;

		VkResult result = vkQueuePresentKHR(vulkan_context->device_manager->getQueue(PRESENT), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			refreshAfterResize();
			return;
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void VulkanEngine::refreshAfterResize() {
		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());

		mainDrawContext->target->resetAccumulatedFrames();
		vulkan_context->swapchain->recreate();
		mainDrawContext->target->recreate(vulkan_context->swapchain->extent);
		guiManager->updateWindows();
		scene_manager->updateScene(mainDrawContext);
	}

	void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t imageIndex) {
		recordBeginCommandBuffer(commandBuffer);
		recordRenderToImage(commandBuffer);
		recordCopyToSwapchain(commandBuffer, imageIndex);
		guiManager->recordGuiCommands(commandBuffer, imageIndex);
		recordEndCommandBuffer(commandBuffer);
	}

	void VulkanEngine::recordBeginCommandBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin record command buffer!");
		}
	}

	void VulkanEngine::recordRenderToImage(VkCommandBuffer commandBuffer) {
		Pipeline pipeline = *scene_manager->getMaterial()->pipeline;

		const uint32_t handleSizeAligned =
				VulkanUtil::alignedSize(DeviceManager::RAYTRACING_PROPERTIES.shaderGroupHandleSize,
										DeviceManager::RAYTRACING_PROPERTIES.shaderGroupHandleAlignment);

		VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
		raygenShaderSbtEntry.deviceAddress = pipeline.raygenShaderBindingTable.deviceAddress;
		raygenShaderSbtEntry.stride = handleSizeAligned;
		raygenShaderSbtEntry.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
		missShaderSbtEntry.deviceAddress = pipeline.missShaderBindingTable.deviceAddress;
		missShaderSbtEntry.stride = handleSizeAligned;
		missShaderSbtEntry.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR closestHitShaderSbtEntry{};
		closestHitShaderSbtEntry.deviceAddress = pipeline.hitShaderBindingTable.deviceAddress;
		closestHitShaderSbtEntry.stride = handleSizeAligned;
		closestHitShaderSbtEntry.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

		std::vector<VkDescriptorSet> descriptor_sets{};
		descriptor_sets.push_back(scene_manager->getSceneDescriptorSet(mainDrawContext->currentFrame));
		descriptor_sets.push_back(scene_manager->getMaterial()->materialDescriptorSet);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getLayoutHandle(), 0,
								static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);

		uint32_t pc_size;
		void *pc_data = properties_manager->getPushConstants(&pc_size, mainDrawContext->target);
		vkCmdPushConstants(commandBuffer, pipeline.getLayoutHandle(),
						   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
								   VK_SHADER_STAGE_MISS_BIT_KHR,
						   0, pc_size, pc_data);

		CmdTraceRaysKHR(vulkan_context->device_manager->getDevice(), commandBuffer, &raygenShaderSbtEntry,
						&missShaderSbtEntry, &closestHitShaderSbtEntry, &callableShaderSbtEntry,
						vulkan_context->swapchain->extent.width, vulkan_context->swapchain->extent.height, 1);
	}

	void VulkanEngine::recordCopyToSwapchain(VkCommandBuffer commandBuffer, const uint32_t swapchain_image_index) {
		AllocatedImage render_target = mainDrawContext->target->getCurrentTargetImage();
		std::shared_ptr<ResourceBuilder> resource_builder = vulkan_context->resource_builder;
		std::shared_ptr<Swapchain> swapchain = vulkan_context->swapchain;

		resource_builder->transitionImageLayout(commandBuffer, swapchain->images[swapchain_image_index],
												VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
												VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		resource_builder->transitionImageLayout(
				commandBuffer, render_target.image, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		int width = swapchain->extent.width;
		int height = swapchain->extent.height;

		VkImageBlit blitRegion{};
		blitRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		blitRegion.srcOffsets[0] = {0, 0, 0};
		blitRegion.srcOffsets[1] = {width, height, 1};
		blitRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		blitRegion.dstOffsets[0] = {0, 0, 0};
		blitRegion.dstOffsets[1] = {width, height, 1};

		vkCmdBlitImage(commandBuffer, render_target.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   swapchain->images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
					   VK_FILTER_NEAREST);

		resource_builder->transitionImageLayout(commandBuffer, swapchain->images[swapchain_image_index],
												VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
												VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		resource_builder->transitionImageLayout(commandBuffer, render_target.image, VK_PIPELINE_STAGE_TRANSFER_BIT,
												VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
												VK_ACCESS_NONE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
												VK_IMAGE_LAYOUT_GENERAL);
	}

	void VulkanEngine::recordEndCommandBuffer(VkCommandBuffer commandBuffer) {
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void VulkanEngine::cleanup() {
		mainDeletionQueue.flush();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VulkanEngine::outputRenderingTarget(const std::string &output_path) {
		AllocatedImage render_target = mainDrawContext->target->getCurrentTargetImage();
		void *data = vulkan_context->resource_builder->downloadImage(render_target, sizeof(uint32_t));
		uint8_t *fixed_data = fixImageFormatForStorage(
				data, render_target.imageExtent.width * render_target.imageExtent.height, render_target.imageFormat);
		vulkan_context->resource_builder->writePNG(output_path, fixed_data, render_target.imageExtent.width,
												   render_target.imageExtent.height);

		delete fixed_data;
	}

	// target format is R8G8B8A8_UNORM
	uint8_t *VulkanEngine::fixImageFormatForStorage(void *data, size_t pixel_count, VkFormat originalFormat) {
		if (originalFormat == VK_FORMAT_R8G8B8A8_UNORM)
			return static_cast<uint8_t *>(data);

		if (originalFormat == VK_FORMAT_B8G8R8A8_UNORM) {
			auto image_data = static_cast<uint8_t *>(data);
			for (size_t i = 0; i < pixel_count; i++) {
				std::swap(image_data[i * 4], image_data[i * 4 + 2]); // Swap B (0) and R (2)
			}
			return image_data;
		}
		if (originalFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
			uint8_t *output_image = new uint8_t[pixel_count * 4];
			auto image_data = static_cast<float *>(data);
			for (size_t i = 0; i < pixel_count * 4; i++) {
				float test = image_data[i];
				// Clamp each channel to the [0, 1] range and then scale to [0, 255]
				output_image[i] = static_cast<uint8_t>(std::fmin(1.0f, std::fmax(0.0f, image_data[i])) * 255);
			}
			delete image_data;
			return output_image;
		} else {
			spdlog::error("Image format of the storage image is not supported to be stored correctly!");
			return nullptr;
		}
	}

	void VulkanEngine::initProperties() {
		renderer_properties = std::make_shared<PropertiesSection>(RENDERER_SECTION_NAME);

		renderer_properties->addString(RESOURCES_DIR_OPTION_NAME, &vulkan_context->base_options->resources_dir);
		renderer_properties->addInt(RECURSION_DEPTH_OPTION_NAME, &vulkan_context->base_options->max_depth, 1, 5);

		initSceneSelectionProperty();
	}

	void VulkanEngine::initSceneSelectionProperty() const
	{
		assert(renderer_properties != nullptr);
		std::string scenes_dir = vulkan_context->base_options->resources_dir + "/scenes";
		std::vector<std::string> scenes;
		try {
			for (const auto &entry: std::filesystem::__cxx11::directory_iterator(scenes_dir)) {
				scenes.push_back(entry.path().filename());
			}
		} catch (const std::exception &e) {
			throw std::runtime_error("failed to load scene directory: " + std::string(e.what()));
		}

		if (scenes.empty()) {
			throw std::runtime_error("No scenes found in scene directory " + scenes_dir + ".");
		}
		vulkan_context->base_options->curr_scene_name = scenes[0];

		renderer_properties->addSelection(CURR_SCENE_OPTION_NAME, &vulkan_context->base_options->curr_scene_name,
										  scenes);
	}

	void VulkanEngine::handleGuiUpdate(uint32_t update_flags) const
	{
		mainDrawContext->target->resetAccumulatedFrames();
		scene_manager->bufferUpdateFlags |= update_flags;
	}

} // namespace RtEngine
