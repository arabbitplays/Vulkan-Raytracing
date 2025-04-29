//
// Created by oschdi on 4/29/25.
//

#ifndef INSTANCEMANAGER_HPP
#define INSTANCEMANAGER_HPP

#include <VulkanContext.hpp>
#include <Material.hpp>
#include <IRenderable.hpp>

namespace RtEngine {
class InstanceManager {
public:
  InstanceManager() = default;
  InstanceManager(std::shared_ptr<ResourceBuilder> resource_builder) : resource_builder(resource_builder) {}

  void createInstanceMappingBuffer(std::vector<RenderObject> &objects);
  void createEmittingInstancesBuffer(std::vector<RenderObject> &objects, std::shared_ptr<Material> material);

  AllocatedBuffer getInstanceBuffer() const;
  AllocatedBuffer getEmittingInstancesBuffer() const;
  uint32_t getEmittingInstancesCount() const;

  void destroy();

private:
  std::shared_ptr<ResourceBuilder> resource_builder;

  AllocatedBuffer instance_mapping_buffer, emitting_instances_buffer;
  uint32_t emitting_instances_count;

};
}

#endif //INSTANCEMANAGER_HPP
