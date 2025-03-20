//
// Created by oschdi on 3/19/25.
//

#ifndef MESHRENDERER_HPP
#define MESHRENDERER_HPP

#include <Component.hpp>
#include <Material.hpp>

class MeshRenderer : public Component {
public:
    MeshRenderer() = default;
    MeshRenderer(std::shared_ptr<Node> node);
    ~MeshRenderer() override = default;

    void OnRender(DrawContext &ctx) override;
    void OnUpdate() override {};

    std::shared_ptr<MeshAsset> meshAsset;
    std::shared_ptr<MaterialInstance> meshMaterial;
};



#endif //MESHRENDERER_HPP
