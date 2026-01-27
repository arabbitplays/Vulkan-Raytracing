#include "DescriptorAllocator.hpp"
#include <cmath>
#include <stdexcept>

namespace RtEngine {
	VkDescriptorPool DescriptorAllocator::getPool(VkDevice device) {
		VkDescriptorPool newPool;
		if (!readyPools.empty()) {
			newPool = readyPools.back();
			readyPools.pop_back();
		} else {
			newPool = createPool(device, setsPerPool, ratios);

			setsPerPool = setsPerPool * GROW_RATIO;
			if (setsPerPool > MAX_SET_COUNT) {
				setsPerPool = MAX_SET_COUNT;
			}
		}

		return newPool;
	}

	VkDescriptorPool DescriptorAllocator::createPool(const VkDevice device, const uint32_t setCount,
													 std::span<DescriptorAllocator::PoolSizeRatio> poolRatios) {
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (auto [type, ratio]: poolRatios) {
			VkDescriptorPoolSize poolSize{.type = type, .descriptorCount = static_cast<uint32_t>(ratio * static_cast<float>(setCount))};
			poolSizes.push_back(poolSize);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = setCount;

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
		return descriptorPool;
	}

	VkDescriptorPool DescriptorAllocator::createPool(const VkDevice device, const std::vector<VkDescriptorPoolSize>& pool_sizes,
													 const VkDescriptorPoolCreateFlags flags) {
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = flags;
		pool_info.maxSets = 1;
		pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		pool_info.pPoolSizes = pool_sizes.data();
		if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		};

		return descriptorPool;
	}

	void DescriptorAllocator::init(VkDevice device, uint32_t initialSetCount, std::span<PoolSizeRatio> poolRatios) {
		ratios.clear();

		for (auto r: poolRatios) {
			ratios.push_back(r);
		}

		VkDescriptorPool newPool = createPool(device, initialSetCount, poolRatios);
		setsPerPool = initialSetCount * GROW_RATIO;

		readyPools.push_back(newPool);
	}

	void DescriptorAllocator::clearPools(VkDevice device) {
		for (auto p: readyPools) {
			vkResetDescriptorPool(device, p, 0);
		}

		for (auto p: fullPools) {
			vkResetDescriptorPool(device, p, 0);
			readyPools.push_back(p);
		}
		fullPools.clear();
	}

	void DescriptorAllocator::destroyPools(VkDevice device) {
		for (auto p: readyPools) {
			vkDestroyDescriptorPool(device, p, nullptr);
		}
		readyPools.clear();

		for (auto p: fullPools) {
			vkDestroyDescriptorPool(device, p, nullptr);
		}
		fullPools.clear();
	}

	VkDescriptorSet DescriptorAllocator::allocate(const VkDevice device, const VkDescriptorSetLayout layout, const void *pNext) {
		VkDescriptorPool poolToUse = getPool(device);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = pNext;
		allocInfo.descriptorPool = poolToUse;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet descriptorSet;
		VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

		if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
			fullPools.push_back(poolToUse);

			poolToUse = getPool(device);
			allocInfo.descriptorPool = poolToUse;

			if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor set!");
			}
		}

		readyPools.push_back(poolToUse);
		return descriptorSet;
	}

	void DescriptorAllocator::writeBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, uint32_t offset,
										  VkDescriptorType type) {
		VkDescriptorBufferInfo &bufferInfo =
				bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &bufferInfo;

		writes.push_back(write);
	}

	void DescriptorAllocator::writeBuffer(uint32_t binding, VkBuffer buffer, uint32_t offset, VkDescriptorType type) {
		writeBuffer(binding, buffer, VK_WHOLE_SIZE, offset, type);
	}

	void DescriptorAllocator::writeImage(uint32_t binding, VkImageView imageView, VkSampler sampler,
										 VkImageLayout layout, VkDescriptorType type) {
		ImageInfoWrapper wrapper{};
		wrapper.image_infos.push_back(
			VkDescriptorImageInfo{.sampler = sampler, .imageView = imageView, .imageLayout = layout});
		imageInfos.push_back(wrapper);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = imageInfos.back().image_infos.data();

		writes.push_back(write);
	}

	void DescriptorAllocator::writeImages(const uint32_t binding, const std::vector<VkImageView>& imageViews, const VkSampler sampler,
										  const VkImageLayout layout, const VkDescriptorType type) {
		ImageInfoWrapper wrapper{};
		wrapper.image_infos.resize(imageViews.size());
		for (size_t i = 0; i < imageViews.size(); i++) {
			wrapper.image_infos[i] = VkDescriptorImageInfo{
					.sampler = sampler, .imageView = imageViews[i], .imageLayout = layout};
		}
		imageInfos.push_back(wrapper);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = static_cast<uint32_t>(imageViews.size());
		write.descriptorType = type;
		write.pImageInfo = imageInfos.back().image_infos.data();

		writes.push_back(write);
	}

	void DescriptorAllocator::writeAccelerationStructure(uint32_t binding,
														 VkAccelerationStructureKHR accelerationStructure,
														 VkDescriptorType type) {
		auto& wrapper = accelerationStructureInfos.emplace_back();

		wrapper.structure = accelerationStructure;

		wrapper.info = VkWriteDescriptorSetAccelerationStructureKHR{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
			.pNext = nullptr,
			.accelerationStructureCount = 1,
			.pAccelerationStructures = &wrapper.structure,
		};

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pNext = &wrapper.info;

		writes.push_back(write);
	}

	void DescriptorAllocator::updateSet(const VkDevice &device, const VkDescriptorSet &set) {
		for (auto &write: writes) {
			write.dstSet = set;
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, VK_NULL_HANDLE);
	}

	void DescriptorAllocator::clearWrites() {
		bufferInfos.clear();
		imageInfos.clear();
		accelerationStructureInfos.clear();
		writes.clear();
	}
} // namespace RtEngine
