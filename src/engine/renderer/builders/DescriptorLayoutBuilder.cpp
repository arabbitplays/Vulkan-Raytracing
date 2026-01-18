#include "DescriptorLayoutBuilder.hpp"
#include <stdexcept>

namespace RtEngine {
	void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count) {
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorCount = descriptor_count;
		layoutBinding.descriptorType = type;
		layoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(layoutBinding);
	}

	VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, uint32_t stageFlags) {
		for (auto &binding: bindings) {
			binding.stageFlags = stageFlags;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		bindings.clear();
		return layout;
	}

	void DescriptorLayoutBuilder::clear() { bindings.clear(); }
} // namespace RtEngine
