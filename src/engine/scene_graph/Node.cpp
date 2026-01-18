#include <Node.hpp>

namespace RtEngine {
	Node::Node() {
		transform = std::make_shared<Transform>();
		components.push_back(transform);
	}

	void Node::refreshTransform(const glm::mat4 &parentMatrix) {
		transform->updateTransforms(parentMatrix);
		for (auto &c: children) {
			c->refreshTransform(transform->getWorldTransform());
		}
	}

	void Node::draw(DrawContext &ctx) {
		for (auto &component: components) {
			component->OnRender(ctx);
		}

		for (auto &c: children) {
			c->draw(ctx);
		}
	}

	void Node::addComponent(const std::shared_ptr<Component> &component) { components.push_back(component); }

	void Node::start() const {
		for (auto &component: components) {
			component->OnStart();
		}
	}

	void Node::update() const {
		for (auto &component: components) {
			component->OnUpdate();
		}
	}

	void Node::destroy() const {
		for (auto &component: components) {
			component->OnDestroy();
		}
	}
} // namespace RtEngine
