#ifndef ASSIMPMODELLOADER_HPP
#define ASSIMPMODELLOADER_HPP

#include <ModelLoader.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace RtEngine {
	class AssimpModelLoader : public ModelLoader {
	public:
		AssimpModelLoader() = default;

	protected:
		void loadData(std::string path, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) override;
		void processNode(aiNode *node, const aiScene *scene, std::vector<Vertex> &vertices,
						 std::vector<uint32_t> &indices);
		void processMesh(aiMesh *mesh, const aiScene *scene, std::vector<Vertex> &vertices,
						 std::vector<uint32_t> &indices);
	};

} // namespace RtEngine
#endif // ASSIMPMODELLOADER_HPP
