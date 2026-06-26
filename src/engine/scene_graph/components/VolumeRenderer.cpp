#include "VolumeRenderer.hpp"
#include <Node.hpp>
#include <Scene.hpp>

#include "VolumeAsset.hpp"

namespace RtEngine {
    void VolumeRenderer::OnStart() {
        mesh_asset = context->mesh_repository->getMesh(mesh_asset_name);
        assert(mesh_asset != nullptr);

        refreshVolumeAsset();

        std::shared_ptr<Material> material = context->scene_manager->getCurrentMaterial();
        vol_material = material->getInstanceByName(material_instance_name);
        assert(vol_material != nullptr);
    }

    void VolumeRenderer::OnRender(DrawContext &ctx) {
        auto shared_node = node.lock();
        if (!shared_node) {
            assert(false);
        }

        glm::mat4 nodeMatrix = shared_node->transform->getWorldTransform();

        ctx.addRenderObject(RenderObject{
            InstanceMappingData{mesh_asset->geometry_id, vol_material->getMaterialIndex(), vol_asset->volume_id + 1},
            // TODO this is weird, id == 0 is used to differentiate no volume
            mesh_asset->accelerationStructure, nodeMatrix, mesh_asset->triangle_count, vol_material->getEmissionPower()
        });
    }

    void VolumeRenderer::initProperties(const std::shared_ptr<IProperties> &config,
                                        const UpdateFlagsHandle &update_flags) {
        if (config->startChild(COMPONENT_NAME)) {
            config->addString("mesh", &mesh_asset_name);
            config->addString("volume", &volume_name);
            config->addString("material_name", &material_instance_name);

            bool update_needed = false;
            update_needed |= config->addFloat("g", &g, -1.0, 1.0);
            update_needed |= config->addFloat("maj", &majorant, 0, 1);
            update_needed |= config->addVector("scattering", &scattering);
            update_needed |= config->addVector("absorption", &absorption);

            if (update_needed && mesh_asset) {
                refreshVolumeAsset();
                update_flags->setFlag(VOLUME_UPDATE);
            }
            config->endChild();
        }
    }

    void VolumeRenderer::refreshVolumeAsset() {
        assert(mesh_asset != nullptr);
        if (volume_name.empty()) {
            vol_asset = context->volume_repository->createHomogenousVolumeAsset(
                absorption, scattering, g, mesh_asset);
        } else {
            vol_asset = context->volume_repository->createHeterogenousVolumeAsset(
                volume_name, absorption, scattering, g, mesh_asset);
        }
    }
} // namespace RtEngine
