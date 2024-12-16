//
// Created by oschdi on 12/16/24.
//

#ifndef MESHASSET_HPP
#define MESHASSET_HPP

#include <bits/shared_ptr.h>
#include "RessourceBuilder.hpp"
#include "AccelerationStructure.hpp"

struct MeshBuffers {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
};

struct MeshSurface {
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset {
    std::string name;
    std::vector<MeshSurface> surfaces;
    MeshBuffers meshBuffers;
    std::shared_ptr<AccelerationStructure> accelerationStructure;
};

#endif //MESHASSET_HPP
