#ifndef RUNTIMECONTEXT_HPP
#define RUNTIMECONTEXT_HPP

#include <MeshRepository.hpp>
#include <TextureRepository.hpp>

#include "ISceneManager.hpp"
#include "VulkanRenderer.hpp"

namespace RtEngine {
	class Material;

	struct EngineContext {
		std::shared_ptr<Window> window; // TODO Replace this by an input manager
		std::shared_ptr<VulkanRenderer> renderer;
		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;

		std::shared_ptr<ISceneManager> scene_manager;
	};
} // namespace RtEngine

#endif // RUNTIMECONTEXT_HPP
