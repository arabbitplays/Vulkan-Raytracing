#include <AccelerationStructure.hpp>
#include <../targets/RenderTargetRepository.hpp>

#ifndef BASICS_IRENDERABLE_HPP
#define BASICS_IRENDERABLE_HPP

namespace RtEngine {
    struct InstanceMappingData {
        uint32_t geometry_id;
        uint32_t material_index;
        uint32_t volume_id;
    };

    struct EmittingInstanceData {
        glm::mat4 model_matrix;
        glm::mat4 normal_matrix; // Precomputed transpose(inverse(mat3(model_matrix))) to transform normals into world space
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
        std::vector<std::shared_ptr<RenderTargetRepository> > target_repositories;

        void nextFrame() {
            for (const auto &target: target_repositories) {
                target->nextImage();

                // Keep in lockstep with metal_rough_raygen.rgen: in combined mode a
                // diff sample leads each ratio block, so it fires when the pre-increment
                // biased count is a multiple of biased_samples_per_diff_sample.
                if (target->getDiffSamplesPerFrame() != 0) {
                    const uint32_t per_diff = target->getBiasedSamplesPerDiffSample();
                    if (per_diff == 0 || target->getAccumulatedFrameCount() % per_diff == 0) {
                        target->incrementAccumulatedDiffFrameCount();
                    }
                }
                if (target->getSamplesPerFrame() != 0)
                    target->incrementAccumulatedFrameCount();
            }
        }

        void clear() {
            emitting_object_count = 0;
            objects.clear();
        }

        void addRenderObject(const RenderObject &render_obj) {
            objects.push_back(render_obj);
            if (render_obj.emitting_power > 0.0f) {
                emitting_object_count++;
            }
        }

        std::vector<RenderObject> &getRenderObjects() {
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
