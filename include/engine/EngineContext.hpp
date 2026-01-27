#ifndef RUNTIMECONTEXT_HPP
#define RUNTIMECONTEXT_HPP

#include <MeshRepository.hpp>
#include <TextureRepository.hpp>

#include "InputManager.hpp"
#include "ISceneManager.hpp"
#include "SwapchainManager.hpp"
#include "RaytracingRenderer.hpp"

namespace RtEngine {
	class Material;

	struct EngineContext {
		std::shared_ptr<Window> window;
		std::shared_ptr<SwapchainManager> swapchain_manager;

		std::shared_ptr<RaytracingRenderer> renderer;
		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;

		std::shared_ptr<ISceneManager> scene_manager;
		std::shared_ptr<InputManager> input_manager;
	};
} // namespace RtEngine

#endif // RUNTIMECONTEXT_HPP
