#include "ModelLoader.hpp"

#include <PathUtil.hpp>
#include <iostream>
#include <stdexcept>

namespace RtEngine {
	MeshAsset ModelLoader::loadMeshAsset(std::string resources_path, std::string path) {
		MeshBuffers meshBuffers{};

		std::string full_path = resources_path + "/" + path;
		loadData(full_path, meshBuffers.vertices, meshBuffers.indices);

		MeshAsset meshAsset{};
		meshAsset.name = PathUtil::getFileName(path);
		meshAsset.path = path;
		meshAsset.meshBuffers = meshBuffers;
		meshAsset.vertex_count = meshBuffers.indices.size();
		meshAsset.triangle_count = meshBuffers.indices.size() / 3;

		meshAsset.instance_data = {};
		return meshAsset;
	}
} // namespace RtEngine
