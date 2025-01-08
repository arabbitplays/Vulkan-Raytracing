//
// Created by oschdi on 1/8/25.
//

#include "AssimpModelLoader.hpp"

void AssimpModelLoader::loadData(std::string path, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
    }

    processNode(scene->mRootNode, scene, vertices, indices);
}

void AssimpModelLoader::processNode(aiNode *node, const aiScene *scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, vertices, indices);
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, vertices, indices);
    }
}

void AssimpModelLoader::processMesh(aiMesh *mesh, const aiScene *scene, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) {
            vertex.texCoord = glm::vec3(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, 0);
        } else {
            vertex.texCoord = glm::vec3(0);
        }
        vertex.color = glm::vec3(1);

        vertices.push_back(vertex);
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
}

