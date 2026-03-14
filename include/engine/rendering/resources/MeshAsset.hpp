#ifndef MESHASSET_HPP
#define MESHASSET_HPP

#include <bits/shared_ptr.h>
#include "AccelerationStructure.hpp"
#include "ResourceBuilder.hpp"

namespace RtEngine {
	struct MeshBuffers {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	struct GeometryData {
		uint32_t vertex_offset = 0;
		uint32_t triangle_offset = 0;
	};

	struct MeshAABB {
		glm::vec3 origin;
		glm::vec3 extent;
	};

	struct MeshAsset {
		std::string name;
		std::string path;
		uint32_t geometry_id;
		uint32_t vertex_count = 0;
		uint32_t triangle_count = 0;
		GeometryData geometry_data;
		MeshBuffers meshBuffers;
		std::shared_ptr<AccelerationStructure> accelerationStructure;

		MeshAABB calcAABB() const {
			glm::vec3 min = meshBuffers.vertices[0].pos;
			glm::vec3 max = min;

			for (auto& vertex : meshBuffers.vertices) {
				min = glm::vec3(std::min(min.x, vertex.pos.x), std::min(min.y, vertex.pos.y), std::min(min.z, vertex.pos.z));
				max = glm::vec3(std::max(max.x, vertex.pos.x), std::max(max.y, vertex.pos.y), std::max(max.z, vertex.pos.z));
			}

			return {min, max - min};
		}
	};

} // namespace RtEngine
#endif // MESHASSET_HPP
