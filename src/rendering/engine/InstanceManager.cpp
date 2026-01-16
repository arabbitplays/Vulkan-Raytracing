//
// Created by oschdi on 4/29/25.
//

#include <InstanceManager.hpp>

namespace RtEngine {
	void InstanceManager::createInstanceMappingBuffer(std::vector<RenderObject> &objects) {
		assert(!objects.empty());

		if (instance_mapping_buffer.handle != VK_NULL_HANDLE) {
			resource_builder->destroyBuffer(instance_mapping_buffer);
		}

		std::vector<InstanceMappingData> instance_datas;
		for (int i = 0; i < objects.size(); i++) {
			instance_datas.push_back(objects[i].instance_data);
		}

		instance_mapping_buffer = resource_builder->stageMemoryToNewBuffer(instance_datas.data(),
																		   instance_datas.size() * sizeof(InstanceMappingData),
																		   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	void InstanceManager::createEmittingInstancesBuffer(const std::vector<RenderObject> &objects) {
		assert(!objects.empty());

		if (emitting_instances_buffer.handle != VK_NULL_HANDLE) {
			resource_builder->destroyBuffer(emitting_instances_buffer);
		}

		std::vector<EmittingInstanceData> emitting_instances;
		for (int i = 0; i < objects.size(); i++) {
			EmittingInstanceData instance_data;
			instance_data.instance_id = i;
			instance_data.model_matrix = objects[i].transform;
			float power = objects[i].emitting_power;
			instance_data.primitive_count = objects[i].primitive_count;
			if (power > 0.0f || (i == objects.size() - 1 && emitting_instances.empty())) {
				emitting_instances.push_back(instance_data);
			}
		}

		emitting_instances_count = emitting_instances.size();
		emitting_instances_buffer = resource_builder->stageMemoryToNewBuffer(
				emitting_instances.data(), emitting_instances.size() * sizeof(EmittingInstanceData),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	AllocatedBuffer InstanceManager::getInstanceBuffer() const {
		assert(instance_mapping_buffer.handle != VK_NULL_HANDLE);
		return instance_mapping_buffer;
	}

	AllocatedBuffer InstanceManager::getEmittingInstancesBuffer() const {
		assert(emitting_instances_buffer.handle != VK_NULL_HANDLE);
		return emitting_instances_buffer;
	}

	uint32_t InstanceManager::getEmittingInstancesCount() const {
		if (emitting_instances_buffer.handle != VK_NULL_HANDLE)
		{
			return emitting_instances_count;
		}

		return 0;
	}

	void InstanceManager::destroy() {
		if (instance_mapping_buffer.handle != VK_NULL_HANDLE)
			resource_builder->destroyBuffer(instance_mapping_buffer);
		if (emitting_instances_buffer.handle != VK_NULL_HANDLE)
			resource_builder->destroyBuffer(emitting_instances_buffer);
	}

} // namespace RtEngine
