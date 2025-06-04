#include <AccelerationStructure.hpp>

#ifndef BASICS_IRENDERABLE_HPP
#define BASICS_IRENDERABLE_HPP

namespace RtEngine {
	struct InstanceData {
		uint32_t geometry_id;
		uint32_t material_index;
	};

	struct EmittingInstanceData {
		glm::mat4 model_matrix;
		uint32_t instance_id;
		uint32_t primitive_count;
		uint32_t padding[2];
	};

	struct RenderObject {
		InstanceData instance_data;
		std::shared_ptr<AccelerationStructure> acceleration_structure;
		glm::mat4 transform;
		uint32_t primitive_count;
	};

	struct DrawContext {
		std::vector<RenderObject> objects;
	};

	class IRenderable {
		virtual void draw(DrawContext &ctx) = 0;
	};
} // namespace RtEngine

#endif // BASICS_IRENDERABLE_HPP
