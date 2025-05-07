#include "MeshRenderer.hpp"
#include <Node.hpp>

namespace RtEngine {
MeshRenderer::MeshRenderer(std::shared_ptr<Node> node) : Component(node) {}

void MeshRenderer::OnRender(DrawContext& ctx)
{
    auto shared_node = node.lock();
    if (!shared_node) {
        assert(false);
    }

    glm::mat4 nodeMatrix = shared_node->transform->getWorldTransform();

    ctx.objects.push_back(RenderObject{InstanceData{meshAsset->geometry_id, meshMaterial->material_index}, meshAsset->accelerationStructure, nodeMatrix, meshAsset->triangle_count});
}

std::shared_ptr<PropertiesManager> MeshRenderer::getProperties()
{
    if (!properties)
    {
        properties = std::make_shared<PropertiesManager>();
    }

    properties->property_sections.clear();

    auto mesh_renderer_section = std::make_shared<PropertiesSection>("MeshRenderer");
    mesh_renderer_section->addInt("material_idx", reinterpret_cast<int32_t*>(&meshMaterial->material_index),PERSISTENT_PROPERTY_FLAG);
    mesh_renderer_section->addString("mesh", &meshAsset->name, PERSISTENT_PROPERTY_FLAG);
    properties->addPropertySection(mesh_renderer_section);

    if (meshMaterial)
    {
        properties->addPropertySection(meshMaterial->properties, SERIALIZABLE_PROPERTY_FLAG);
    }

    return properties;
}
}
