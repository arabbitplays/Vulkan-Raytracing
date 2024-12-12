//
// Created by oster on 12.09.2024.
//

#ifndef BASICS_MESHNODE_HPP
#define BASICS_MESHNODE_HPP


#include "Node.hpp"
#include "../../builders/MeshAssetBuilder.hpp"

struct MeshNode : public Node {
public:
    MeshNode() = default;

    std::shared_ptr<MeshAsset> meshAsset;
    std::shared_ptr<Material> material;

    void draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};


#endif //BASICS_MESHNODE_HPP
