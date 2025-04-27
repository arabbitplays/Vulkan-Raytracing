#ifndef MESHASSET_HPP
#define MESHASSET_HPP

#include <bits/shared_ptr.h>
#include "ResourceBuilder.hpp"
#include "AccelerationStructure.hpp"

namespace RtEngine {
struct MeshBuffers {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct GeometryData {
    uint32_t vertex_offset = 0;
    uint32_t triangle_offset = 0;
};

struct MeshAsset {
    std::string name;
    std::string path;
    uint32_t geometry_id;
    uint32_t vertex_count = 0;
    uint32_t triangle_count = 0;
    GeometryData instance_data;
    MeshBuffers meshBuffers;
    std::shared_ptr<AccelerationStructure> accelerationStructure;
};

}
#endif //MESHASSET_HPP
