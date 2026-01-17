#include "VulkanRenderer.hpp"

#include "QuickTimer.hpp"
#include "SceneUtil.hpp"

namespace RtEngine {
    VulkanRenderer::VulkanRenderer(std::shared_ptr<VulkanContext> vulkan_context) : vulkan_context(vulkan_context) {
        geometry_manager = std::make_shared<GeometryManager>(vulkan_context);
    }

    void VulkanRenderer::updateStaticGeometry(std::shared_ptr<Scene> scene) {
        std::vector<std::shared_ptr<MeshAsset>> mesh_assets = SceneUtil::collectMeshAssets(scene->getRootNode());
        geometry_manager->createGeometryBuffers(mesh_assets);

        // remove this here
        scene->material->writeMaterial();

        vulkan_context->descriptor_allocator->writeBuffer(3, geometry_manager->getVertexBuffer().handle, 0,
                                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vulkan_context->descriptor_allocator->writeBuffer(4, geometry_manager->getIndexBuffer().handle, 0,
                                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vulkan_context->descriptor_allocator->writeBuffer(5, geometry_manager->getGeometryMappingBuffer().handle, 0,
                                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }

    void VulkanRenderer::clearGeometry() {
        geometry_manager->destroy();
    }
}

