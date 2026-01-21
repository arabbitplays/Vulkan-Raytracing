#ifndef BASICS_DESCRIPTORALLOCATOR_HPP
#define BASICS_DESCRIPTORALLOCATOR_HPP

#include <list>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace RtEngine {
	class DescriptorAllocator {
	public:
		struct PoolSizeRatio {
			VkDescriptorType type;
			float ratio;
		};

		struct ImageInfoWrapper
		{
			std::vector<VkDescriptorImageInfo> image_infos;
		};

		struct AccelerationStructureInfoWrapper {
			VkAccelerationStructureKHR structure;
			VkWriteDescriptorSetAccelerationStructureKHR info;
		};

		void init(VkDevice device, uint32_t initialSetCount, std::span<PoolSizeRatio> poolRatios);
		void clearPools(VkDevice device);
		void destroyPools(VkDevice device);
		VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, const void *pNext = nullptr);
		static VkDescriptorPool createPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& pool_sizes,
									VkDescriptorPoolCreateFlags flags);

		void writeBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, uint32_t offset, VkDescriptorType type);
		void writeBuffer(uint32_t binding, VkBuffer buffer, uint32_t offset, VkDescriptorType type);
		void writeImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout,
						VkDescriptorType type);
		void writeImages(uint32_t binding, const std::vector<VkImageView>& imageViews, VkSampler sampler, VkImageLayout layout,
						 VkDescriptorType type);
		void writeAccelerationStructure(uint32_t binding, VkAccelerationStructureKHR accelerationStructure,
		                                VkDescriptorType type);
		void updateSet(const VkDevice &device, const VkDescriptorSet &set);
		void clearWrites();

		VkDescriptorPool getGUIDescriptorPool();

	private:
		VkDescriptorPool getPool(VkDevice device);
		static VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

		std::vector<PoolSizeRatio> ratios;
		std::vector<VkDescriptorPool> fullPools;
		std::vector<VkDescriptorPool> readyPools;
		uint32_t setsPerPool;

		std::list<ImageInfoWrapper> imageInfos{};
		std::list<VkDescriptorBufferInfo> bufferInfos;
		std::list<AccelerationStructureInfoWrapper> accelerationStructureInfos;
		std::vector<VkWriteDescriptorSet> writes;

		const uint32_t GROW_RATIO = 2;
		const uint32_t MAX_SET_COUNT = 4092;
	};

} // namespace RtEngine
#endif // BASICS_DESCRIPTORALLOCATOR_HPP
