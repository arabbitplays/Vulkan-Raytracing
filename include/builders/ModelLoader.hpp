//
// Created by oschdi on 1/8/25.
//

#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP

#include <vector>
#include <Vertex.hpp>
#include <string.h>
#include <MeshAsset.hpp>


class ModelLoader {
public:
    ModelLoader() = default;
    virtual ~ModelLoader() = default;

    MeshAsset loadMeshAsset(std::string ressources_path, std::string path);

protected:
    virtual void loadData(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) = 0;
};



#endif //MODELLOADER_HPP
