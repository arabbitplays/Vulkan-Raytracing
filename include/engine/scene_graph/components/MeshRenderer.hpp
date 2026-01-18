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

		void OnStart() override {};
		void OnRender(DrawContext &ctx) override;
		void OnUpdate() override {};
		void OnDestroy() override {};

		void definePropertySections() override;
		void initProperties(const YAML::Node &config_node) override;

		std::shared_ptr<MeshAsset> meshAsset;
		std::shared_ptr<MaterialInstance> meshMaterial;
	};

} // namespace RtEngine
#endif // MESHRENDERER_HPP
