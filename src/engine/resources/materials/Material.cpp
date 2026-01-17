#include "Material.hpp"

namespace RtEngine {
	AllocatedBuffer Material::createMaterialBuffer() {
		std::vector<void*> resource_ptrs(instances.size());
		std::vector<size_t> sizes(instances.size());
		size_t total_size = 0;
		std::vector<std::shared_ptr<MaterialInstance>> instance_list = getInstances();
		for (uint32_t i = 0; i < instance_list.size(); i++) {
			resource_ptrs[i] = instance_list[i]->getResources(&sizes[i], material_textures);
			instance_list[i]->setMaterialIndex(i);
			total_size += sizes[i];
		}

		const auto material_data = static_cast<std::byte*>(std::malloc(total_size));
		std::byte* dst = material_data;
		for (uint32_t i = 0; i < resource_ptrs.size(); i++) {
			std::memcpy(dst, resource_ptrs[i], sizes[i]);
			dst += sizes[i];
		}

		AllocatedBuffer material_buffer = vulkan_context->resource_builder->stageMemoryToNewBuffer(
				material_data, total_size,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		free(material_data);
		return material_buffer;
	}

	std::vector<std::shared_ptr<MaterialInstance>> Material::getInstances() {
		std::vector<std::shared_ptr<MaterialInstance>> result;
		result.reserve(instances.size());

		for (const auto& pair : instances) {
			result.push_back(pair.second);
		}
		return result;
	}

	std::shared_ptr<MaterialInstance> Material::getInstanceByName(const std::string& name) {
		return instances[name];
	}

	std::shared_ptr<PropertiesSection> Material::getProperties() {
		if (properties == nullptr) {
			initProperties();
		}

		assert(properties != nullptr);
		return properties;
	}

	void Material::clearResources() {
		resetQueue.flush();
		mainDeletionQueue.flush();
		if (material_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(material_buffer);
			material_buffer.handle = VK_NULL_HANDLE;
		}

		material_textures->clear();
	}

	void Material::reset() {
		resetQueue.flush();
		if (material_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(material_buffer);
			material_buffer.handle = VK_NULL_HANDLE;
		}

		material_textures->clear();
	}
} // namespace RtEngine
