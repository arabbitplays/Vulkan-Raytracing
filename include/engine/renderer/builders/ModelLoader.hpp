#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP

#include <MeshAsset.hpp>
#include <../renderer/resources/Vertex.hpp>
#include <string.h>
#include <vector>

namespace RtEngine {
	class ModelLoader {
	public:
		ModelLoader() = default;
		virtual ~ModelLoader() = default;

		MeshAsset loadMeshAsset(std::string ressources_path, std::string path);

	protected:
		virtual void loadData(std::string path, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) = 0;
	};

} // namespace RtEngine
#endif // MODELLOADER_HPP
