#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include "EngineContext.hpp"

namespace RtEngine {
	class Node;

	static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;

	class Component : public ISerializable {
	public:
		Component() = default;
		Component(const std::shared_ptr<EngineContext>& context, const std::shared_ptr<Node>& node) : node(node), context(context){};
		virtual ~Component() = default;

		virtual void OnStart() = 0;
		virtual void OnRender(DrawContext &ctx) = 0;
		virtual void OnUpdate() = 0;
		virtual void OnDestroy() = 0;

		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override = 0;

		std::weak_ptr<Node> node;
		std::shared_ptr<EngineContext> context;
	};

} // namespace RtEngine
#endif // COMPONENT_HPP
