//
// Created by oschdi on 5/19/25.
//

#ifndef RUNTIMECONTEXT_HPP
#define RUNTIMECONTEXT_HPP

#include <MeshRepository.hpp>
#include <TextureRepository.hpp>

namespace RtEngine {
	class Material;

	struct RuntimeContext {
		std::shared_ptr<TextureRepository<>> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;

		std::weak_ptr<Material> curr_material;
	};
} // namespace RtEngine

#endif // RUNTIMECONTEXT_HPP
