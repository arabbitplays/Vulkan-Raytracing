#include "Material.hpp"

namespace RtEngine {
	std::vector<std::shared_ptr<MaterialInstance>> Material::getInstances() { return instances; }

	std::shared_ptr<PropertiesSection> Material::getProperties() {
		if (properties == nullptr) {
			initProperties();
		}

		assert(properties != nullptr);
		return properties;
	}

	void Material::clearRessources() {
		resetQueue.flush();
		mainDeletionQueue.flush();
		if (material_buffer.handle != VK_NULL_HANDLE)
			vulkan_context->resource_builder->destroyBuffer(material_buffer);
	}

	void Material::reset() { resetQueue.flush(); }
} // namespace RtEngine
