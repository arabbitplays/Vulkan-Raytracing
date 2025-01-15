//
// Created by oschdi on 1/8/25.
//

#ifndef ASSIMPMODELLOADER_HPP
#define ASSIMPMODELLOADER_HPP

#include <ModelLoader.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class AssimpModelLoader : public ModelLoader {
public:
    AssimpModelLoader() = default;

protected:
    void loadData(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) override;
    void processNode(aiNode *node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void processMesh(aiMesh *mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};



#endif //ASSIMPMODELLOADER_HPP
