#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <PropertiesManager.hpp>
#include <RuntimeContext.hpp>
#include <IRenderable.hpp>

namespace RtEngine {
	class Node;

	static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;

	class Component {
	public:
		Component() = default;
		Component(const std::shared_ptr<RuntimeContext>& context, const std::shared_ptr<Node>& node) : node(node), context(context){};
		virtual ~Component() = default;

		virtual void OnStart() = 0;
		virtual void OnRender(DrawContext &ctx) = 0;
		virtual void OnUpdate() = 0;

		virtual void definePropertySections() = 0;
		std::shared_ptr<PropertiesManager> getProperties() {
			if (properties == nullptr) {
				properties = std::make_shared<PropertiesManager>();

				properties->property_sections.clear();
				definePropertySections();
			}

			return properties;
		};
		virtual void initProperties(const YAML::Node &config_node) {
			properties = std::make_shared<PropertiesManager>(config_node);
			definePropertySections();
		};

		std::weak_ptr<Node> node;
		std::shared_ptr<RuntimeContext> context;

	protected:
		std::shared_ptr<PropertiesManager> properties;
	};

} // namespace RtEngine
#endif // COMPONENT_HPP
