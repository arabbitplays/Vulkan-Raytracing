//
// Created by oschdi on 4/29/25.
//

#ifndef INSTANCEMANAGER_HPP
#define INSTANCEMANAGER_HPP

#include <Material.hpp>

#include "../resources/IRenderable.hpp"


namespace RtEngine {
	class InstanceManager {
	public:
		InstanceManager() = default;
		InstanceManager(std::shared_ptr<ResourceBuilder> resource_builder) : resource_builder(resource_builder) {}

		void createInstanceMappingBuffer(std::vector<RenderObject> &objects);
		void createEmittingInstancesBuffer(const std::vector<RenderObject> &objects);

		AllocatedBuffer getInstanceBuffer() const;
		AllocatedBuffer getEmittingInstancesBuffer() const;
		uint32_t getEmittingInstancesCount() const;

		void destroy();

	private:
		std::shared_ptr<ResourceBuilder> resource_builder;

		AllocatedBuffer instance_mapping_buffer, emitting_instances_buffer;
		uint32_t emitting_instances_count;
	};
} // namespace RtEngine

#endif // INSTANCEMANAGER_HPP
