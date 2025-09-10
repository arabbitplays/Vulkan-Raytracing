#include "Material.hpp"

namespace RtEngine {
	std::shared_ptr<PropertiesSection> Material::getProperties() {
		if (properties == nullptr) {
			initProperties();
		}

		assert(properties != nullptr);
		return properties;
	}

	void Material::destroyResources() {
		mainDeletionQueue.flush();
		destroyLayout();
	}

	void Material::destroyLayout() {
		vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), descriptor_layout, nullptr);
	}

	void Material::reset() { }
} // namespace RtEngine
