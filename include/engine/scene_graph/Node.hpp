#ifndef BASICS_NODE_HPP
#define BASICS_NODE_HPP

#include <../renderer/resources/IRenderable.hpp>
#include <Transform.hpp>

namespace RtEngine {
	class Node : public IRenderable {
	public:
		Node();
		~Node() = default;

		void draw(DrawContext &ctx) override;
		void refreshTransform(const glm::mat4 &parentMatrix);
		void addComponent(const std::shared_ptr<Component> &component);
		template<typename T>
		std::shared_ptr<T> getComponent() {
			for (auto &component: components) {
				auto casted = std::dynamic_pointer_cast<T>(component);
				if (casted) {
					return casted;
				}
			}
			return nullptr;
		}

		void start() const;
		void update() const;
		void destroy() const;

		std::string name;

		std::weak_ptr<Node> parent;
		std::vector<std::shared_ptr<Node>> children;

		std::vector<std::shared_ptr<Component>> components;
		std::shared_ptr<Transform> transform;
	};

} // namespace RtEngine
#endif // BASICS_NODE_HPP
