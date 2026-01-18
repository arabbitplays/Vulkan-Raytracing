#ifndef MESHRENDERER_HPP
#define MESHRENDERER_HPP

#include <Component.hpp>
#include <Material.hpp>

namespace RtEngine {
	class MeshRenderer : public Component {
	public:
		MeshRenderer() = default;
		MeshRenderer(const std::shared_ptr<EngineContext>& context, const std::shared_ptr<Node>& node) : Component(context, node){};

		static constexpr std::string COMPONENT_NAME = "MeshRenderer";

		void OnStart() override;
		void OnRender(DrawContext &ctx) override;
		void OnUpdate() override {};
		void OnDestroy() override {};

		void definePropertySections() override;
		void initProperties(const YAML::Node &config_node) override;

		std::shared_ptr<MeshAsset> mesh_asset;
		std::shared_ptr<MaterialInstance> mesh_material;

	private:
		std::string mesh_asset_name;
		std::string material_instance_name;
	};

} // namespace RtEngine
#endif // MESHRENDERER_HPP
