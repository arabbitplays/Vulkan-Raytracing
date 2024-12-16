//
// Created by oschdi on 12/16/24.
//

#ifndef MESHASSET_HPP
#define MESHASSET_HPP

#include <bits/shared_ptr.h>
#include "RessourceBuilder.hpp"
#include "AccelerationStructure.hpp"

struct MeshBuffers {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct InstanceData {
    uint32_t vertex_offset = 0;
    uint32_t triangle_offset = 0;
};

struct MeshAsset {
    std::string name;
    uint32_t vertex_count = 0;
    uint32_t triangle_count = 0;
    InstanceData instance_data;
    MeshBuffers meshBuffers;
    std::shared_ptr<AccelerationStructure> accelerationStructure;
};

#endif //MESHASSET_HPP
