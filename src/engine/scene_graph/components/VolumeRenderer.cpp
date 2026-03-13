#include "VolumeRenderer.hpp"
#include <Node.hpp>
#include <Scene.hpp>

#include "VolumeAsset.hpp"

namespace RtEngine {
    void VolumeRenderer::OnStart() {
        mesh_asset = context->mesh_repository->getMesh(mesh_asset_name);
        assert(mesh_asset != nullptr);

        // vol_asset = context->volume_repository->getVolume(volume_asset_name);
        vol_asset = std::make_shared<VolumeAsset>();
        vol_asset->name = material_instance_name;
        vol_asset->isHomogenous = true;
        vol_asset->volume_data = { 0.01, 0.02 };

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

        ctx.addRenderObject(RenderObject{InstanceMappingData{mesh_asset->geometry_id, vol_material->getMaterialIndex(), vol_asset->volume_id + 1},  // TODO this is weird, id == 0 is used to differentiate no volume
                                           mesh_asset->accelerationStructure, nodeMatrix, mesh_asset->triangle_count, vol_material->getEmissionPower()});
    }

    void VolumeRenderer::initProperties(const std::shared_ptr<IProperties> &config,
        const UpdateFlagsHandle &update_flags) {

        if (config->startChild(COMPONENT_NAME)) {
            config->addString("mesh", &mesh_asset_name);
            config->addString("volume", &volume_asset_name);
            config->addString("material_name", &material_instance_name);

            if (vol_asset) {
                bool update_needed = false;
                update_needed |= config->addFloat("scattering", &vol_asset->volume_data.scattering);
                update_needed |= config->addFloat("absorption", &vol_asset->volume_data.absorption);
                if (update_needed) {
                    update_flags->setFlag(VOLUME_UPDATE);
                }
            }
            config->endChild();
        }
    }

} // namespace RtEngine
