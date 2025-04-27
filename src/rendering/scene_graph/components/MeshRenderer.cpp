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

    properties->properties.clear();
    if (meshMaterial)
    {
        properties->addPropertySection(meshMaterial->properties);
    }

    return properties;
}
}
