#include "MeshAssetBuilder.hpp"
#include <iostream>
#include <stdexcept>

#include <AssimpModelLoader.hpp>
#include <ModelLoader.hpp>
#include <cstring>

namespace RtEngine {
	MeshAsset MeshAssetBuilder::loadMeshAsset(std::string path) {
		AssimpModelLoader loader;
		return loader.loadMeshAsset(resource_path, path);
	}

	void MeshAssetBuilder::destroyMeshAsset(MeshAsset &meshAsset) {
		// intentional empty
	}
} // namespace RtEngine
