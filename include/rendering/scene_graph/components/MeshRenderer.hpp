#ifndef MESHRENDERER_HPP
#define MESHRENDERER_HPP

#include <Component.hpp>
#include <Material.hpp>

namespace RtEngine {
class MeshRenderer : public Component {
public:
    MeshRenderer() = default;
    MeshRenderer(std::shared_ptr<Node> node);

    void OnStart() override {};
    void OnRender(DrawContext &ctx) override;
    void OnUpdate() override {};
    std::shared_ptr<PropertiesManager> getProperties() override;

    std::shared_ptr<MeshAsset> meshAsset;
    std::shared_ptr<MaterialInstance> meshMaterial;
};



}
#endif //MESHRENDERER_HPP
