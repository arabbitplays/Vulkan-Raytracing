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

    MeshAsset LoadMeshAsset(std::string name, std::string path);

protected:
    virtual void loadData(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) = 0;
};



#endif //MODELLOADER_HPP
