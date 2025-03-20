//
// Created by oschdi on 3/19/25.
//

#ifndef MESHRENDERER_HPP
#define MESHRENDERER_HPP

#include <Component.hpp>

class MeshRenderer : Component {
    MeshRenderer() = default;
    MeshRenderer(std::shared_ptr<Node> node);

    void OnRender(DrawContext &ctx) override;
};



#endif //MESHRENDERER_HPP
