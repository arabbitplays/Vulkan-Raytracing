#ifndef BASICS_DESCRIPTORLAYOUTBUILDER_HPP
#define BASICS_DESCRIPTORLAYOUTBUILDER_HPP

#include <vector>
#include <vulkan/vulkan_core.h>

namespace RtEngine {
	class DescriptorLayoutBuilder {
	public:
		void addBinding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count = 1);
		VkDescriptorSetLayout build(VkDevice device, uint32_t stageFlags);
		void clear();

	private:
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

} // namespace RtEngine
#endif // BASICS_DESCRIPTORLAYOUTBUILDER_HPP
