//
// Created by oschdi on 5/19/25.
//

#ifndef RUNTIMECONTEXT_HPP
#define RUNTIMECONTEXT_HPP

#include <MeshRepository.hpp>
#include <TextureRepository.hpp>

namespace RtEngine {
struct RuntimeContext {
    std::shared_ptr<TextureRepository> texture_repository;
    std::shared_ptr<MeshRepository> mesh_repository;
};
}

#endif //RUNTIMECONTEXT_HPP
