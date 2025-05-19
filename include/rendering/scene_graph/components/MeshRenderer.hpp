#ifndef MESHRENDERER_HPP
#define MESHRENDERER_HPP

#include <Component.hpp>
#include <Material.hpp>

namespace RtEngine {
class MeshRenderer : public Component {
public:
    MeshRenderer() = default;
    MeshRenderer(std::shared_ptr<RuntimeContext> context, std::shared_ptr<Node> node) : Component(context, node) {};

    static constexpr std::string COMPONENT_NAME = "MeshRenderer";

    void OnStart() override {};
    void OnRender(DrawContext &ctx) override;
    void OnUpdate() override {};
    void definePropertySections() override;

    std::shared_ptr<MeshAsset> meshAsset;
    std::shared_ptr<MaterialInstance> meshMaterial;
};



}
#endif //MESHRENDERER_HPP
