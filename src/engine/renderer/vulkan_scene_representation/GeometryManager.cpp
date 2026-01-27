#include <GeometryManager.hpp>
#include <MeshRenderer.hpp>

#include "QuickTimer.hpp"

namespace RtEngine {

	void GeometryManager::createGeometryBuffers(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) {
		QuickTimer timer{"Static geometry", true};

		vertex_buffer = createVertexBuffer(mesh_assets);
		index_buffer = createIndexBuffer(mesh_assets);
		geometry_mapping_buffer = createGeometryMappingBuffer(mesh_assets);
		createBlas(mesh_assets);
	}

	void GeometryManager::writeGeometryBuffers() const {
		vulkan_context->descriptor_allocator->writeBuffer(3, vertex_buffer.handle, 0,
												  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		vulkan_context->descriptor_allocator->writeBuffer(4, index_buffer.handle, 0,
														  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		vulkan_context->descriptor_allocator->writeBuffer(5, geometry_mapping_buffer.handle, 0,
														  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	AllocatedBuffer GeometryManager::createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const {
		assert(!mesh_assets.empty());

		if (vertex_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(vertex_buffer);
		}

		VkDeviceSize size = 0;
		for (auto &mesh_asset: mesh_assets) {
			size += mesh_asset->meshBuffers.vertices.size();
		}

		std::vector<Vertex> vertices(size);

		uint32_t vertex_offset = 0;
		for (auto &mesh_asset: mesh_assets) {
			vertices.insert(vertices.begin() + vertex_offset, mesh_asset->meshBuffers.vertices.begin(),
							mesh_asset->meshBuffers.vertices.end());

			mesh_asset->instance_data.vertex_offset = vertex_offset;
			vertex_offset += mesh_asset->meshBuffers.vertices.size();
		}

		return vulkan_context->resource_builder->stageMemoryToNewBuffer(
				vertices.data(), vertices.size() * sizeof(Vertex),
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	AllocatedBuffer GeometryManager::createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const {
		assert(!mesh_assets.empty());

		if (index_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(index_buffer);
		}

		VkDeviceSize size = 0;
		for (auto &mesh_asset: mesh_assets) {
			size += mesh_asset->meshBuffers.indices.size();
		}

		std::vector<uint32_t> indices(size);

		uint32_t index_offset = 0;
		for (auto &mesh_asset: mesh_assets) {
			indices.insert(indices.begin() + index_offset, mesh_asset->meshBuffers.indices.begin(),
						   mesh_asset->meshBuffers.indices.end());

			mesh_asset->instance_data.triangle_offset = index_offset;
			index_offset += mesh_asset->meshBuffers.indices.size();
		}

		return vulkan_context->resource_builder->stageMemoryToNewBuffer(
				indices.data(), indices.size() * sizeof(uint32_t),
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	AllocatedBuffer
	GeometryManager::createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>> &mesh_assets) const {
		assert(!mesh_assets.empty());

		if (geometry_mapping_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(geometry_mapping_buffer);
		}

		std::vector<GeometryData> geometry_datas;
		for (auto &mesh_asset: mesh_assets) {
			geometry_datas.push_back(mesh_asset->instance_data);
		}
		return vulkan_context->resource_builder->stageMemoryToNewBuffer(
				geometry_datas.data(), mesh_assets.size() * sizeof(GeometryData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	void GeometryManager::createBlas(std::vector<std::shared_ptr<MeshAsset>> &meshes) {
		assert(vertex_buffer.handle != VK_NULL_HANDLE && index_buffer.handle != VK_NULL_HANDLE);

		for (auto& structure : blas) {
			structure->destroy();
		}
		blas.clear();

		VkDevice device = vulkan_context->device_manager->getDevice();

		uint32_t object_id = 0;
		for (auto &meshAsset: meshes) {
			meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(
					device, *vulkan_context->resource_builder, *vulkan_context->command_manager,
					VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

			meshAsset->accelerationStructure->addTriangleGeometry(
					vertex_buffer, index_buffer,
					meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
					meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
			meshAsset->accelerationStructure->build();
			meshAsset->geometry_id = object_id++;

			blas.push_back(meshAsset->accelerationStructure);
		}
	}


	void GeometryManager::destroy() {
		if (vertex_buffer.handle != VK_NULL_HANDLE)
			vulkan_context->resource_builder->destroyBuffer(vertex_buffer);
		if (index_buffer.handle != VK_NULL_HANDLE)
			vulkan_context->resource_builder->destroyBuffer(index_buffer);
		if (geometry_mapping_buffer.handle != VK_NULL_HANDLE)
			vulkan_context->resource_builder->destroyBuffer(geometry_mapping_buffer);

		for (auto& structure : blas) {
			structure->destroy();
		}
		blas.clear();
	}

} // namespace RtEngine
