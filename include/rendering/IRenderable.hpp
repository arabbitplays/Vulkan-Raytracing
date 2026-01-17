#include <AccelerationStructure.hpp>
#include <RenderTarget.hpp>

#ifndef BASICS_IRENDERABLE_HPP
#define BASICS_IRENDERABLE_HPP

namespace RtEngine {
	struct InstanceMappingData {
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
		InstanceMappingData instance_mapping_data;
		std::shared_ptr<AccelerationStructure> acceleration_structure;
		glm::mat4 transform;
		uint32_t primitive_count;
		float emitting_power;
	};

	struct DrawContext {
		uint32_t currentFrame = 0;
		uint32_t max_frames_in_flight = 1;
		std::shared_ptr<RenderTarget> target;

		void nextFrame()
		{
			currentFrame = (currentFrame + 1) % max_frames_in_flight;
			target->nextImage();
		}

		void clear() {
			emitting_object_count = 0;
			objects.clear();
		}

		void addRenderObject(const RenderObject& render_obj) {
			objects.push_back(render_obj);
			if (render_obj.emitting_power > 0.0f) {
				emitting_object_count++;
			}
		}

		std::vector<RenderObject>& getRenderObjects() {
			return objects;
		}

		uint32_t getEmittingObjectCount() const {
			return emitting_object_count;
		}

	private:
		std::vector<RenderObject> objects;
		uint32_t emitting_object_count = 0;
	};

	class IRenderable {
		virtual void draw(DrawContext &ctx) = 0;
	};
} // namespace RtEngine

#endif // BASICS_IRENDERABLE_HPP
