#ifndef GEOMETRYMANAGER_HPP
#define GEOMETRYMANAGER_HPP

#include <Scene.hpp>
#include <VulkanContext.hpp>

namespace RtEngine {

	class GeometryManager {
	public:
		GeometryManager() = default;
		GeometryManager(const std::shared_ptr<VulkanContext> &vulkan_context) :
			vulkan_context(vulkan_context) {}

		void createGeometryBuffers(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets);

		AllocatedBuffer getVertexBuffer() const;
		AllocatedBuffer getIndexBuffer() const;
		AllocatedBuffer getGeometryMappingBuffer() const;

		void destroy();

	private:
		AllocatedBuffer createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const;
		AllocatedBuffer createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const;
		AllocatedBuffer createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const;
		void createBlas(std::vector<std::shared_ptr<MeshAsset>> &meshes);

		std::shared_ptr<VulkanContext> vulkan_context;
		AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer;
		std::vector<std::shared_ptr<AccelerationStructure>> blas;
	};
} // namespace RtEngine

#endif // GEOMETRYMANAGER_HPP
