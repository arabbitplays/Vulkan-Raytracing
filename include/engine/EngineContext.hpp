#ifndef RUNTIMECONTEXT_HPP
#define RUNTIMECONTEXT_HPP

#include <MeshRepository.hpp>
#include <TextureRepository.hpp>

#include "VulkanRenderer.hpp"

namespace RtEngine {
	class Material;

	struct EngineContext {
		std::shared_ptr<VulkanRenderer> renderer;
		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;
	};
} // namespace RtEngine

#endif // RUNTIMECONTEXT_HPP
