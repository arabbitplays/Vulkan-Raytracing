//
// Created by oschdi on 5/4/25.
//

#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP
#include <Component.hpp>

namespace RtEngine {
	class Rigidbody : public Component {
	public:
		Rigidbody() = default;
		explicit Rigidbody(const std::shared_ptr<Node>& node) : Component(nullptr, node){};

		static constexpr std::string COMPONENT_NAME = "Rigidbody";

		void OnStart() override {};
		void OnRender(DrawContext &ctx) override {};
		void OnUpdate() override;
		void OnDestroy() override {}

		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;

	private:
		float gravity = 0.0f;
	};
} // namespace RtEngine

#endif // RIGIDBODY_HPP
