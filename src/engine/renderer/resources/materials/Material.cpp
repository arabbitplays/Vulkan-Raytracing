#include "Material.hpp"

namespace RtEngine {
	std::vector<std::shared_ptr<MaterialInstance>> Material::getInstances() {
		std::vector<std::shared_ptr<MaterialInstance>> result;
		result.reserve(instances.size());

		for (const auto& pair : instances) {
			result.push_back(pair.second);
		}
		return result;
	}

	std::shared_ptr<MaterialInstance> Material::getInstanceByName(const std::string& instance_name) {
		if (!instances.contains(instance_name))
			return nullptr;
		return instances[instance_name];
	}

	std::shared_ptr<PropertiesSection> Material::getProperties() {
		if (properties == nullptr) {
			initProperties();
		}

		assert(properties != nullptr);
		return properties;
	}

	void Material::clearResources() {
		mainDeletionQueue.flush();
	}

	void Material::reset() {
	}
} // namespace RtEngine
