#ifndef VULKAN_RAYTRACING_VOLUMERENDERER_HPP
#define VULKAN_RAYTRACING_VOLUMERENDERER_HPP
#include "Component.hpp"

namespace RtEngine {
    struct VolumeAsset;

    class VolumeRenderer : public Component {
    public:
        VolumeRenderer() = default;
        VolumeRenderer(const std::shared_ptr<EngineContext>& context, const std::shared_ptr<Node>& node) : Component(context, node){};

        static constexpr std::string COMPONENT_NAME = "VolumeRenderer";

        void OnStart() override;
        void OnRender(DrawContext &ctx) override;
        void OnUpdate() override {};
        void OnDestroy() override {};

        void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;

        std::shared_ptr<VolumeAsset> vol_asset;
        std::shared_ptr<MeshAsset> mesh_asset;
        std::shared_ptr<MaterialInstance> vol_material;

    private:
        std::string mesh_asset_name;
        std::string volume_asset_name;
        std::string material_instance_name;

        float absorption = 0.02f, scattering = 0.5f, g = 0.0f;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMERENDERER_HPP