#ifndef GEOMETRYMANAGER_HPP
#define GEOMETRYMANAGER_HPP

#include <memory>
#include <VulkanContext.hpp>

#include "VolumeAsset.hpp"

namespace RtEngine {

	class GeometryManager {
	public:
		GeometryManager() = default;
		explicit GeometryManager(const std::shared_ptr<VulkanContext> &vulkan_context) :
			vulkan_context(vulkan_context) {}

		void createGeometryBuffers(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets);
		void writeGeometryBuffers() const;

		void destroy();

	private:
		AllocatedBuffer createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const;
		AllocatedBuffer createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const;
		AllocatedBuffer createGeometryMappingBuffer(const std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const;

		AllocatedBuffer createVolumeBuffer(std::vector<std::shared_ptr<VolumeAsset>> &volume_assets) const;

		void createBlas(std::vector<std::shared_ptr<MeshAsset>> &meshes);

		std::shared_ptr<VulkanContext> vulkan_context;
		AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer;
		std::vector<std::shared_ptr<AccelerationStructure>> blas;
	};
} // namespace RtEngine

#endif // GEOMETRYMANAGER_HPP
