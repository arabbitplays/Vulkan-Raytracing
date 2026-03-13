#include "VolumeManager.hpp"

#include "QuickTimer.hpp"

namespace RtEngine {
    void VolumeManager::createVolumeBuffer(const std::vector<std::shared_ptr<VolumeAsset>> &volume_assets) {
        assert(!volume_assets.empty());

        QuickTimer timer{"Volumes", true};

        if (volume_buffer.handle != VK_NULL_HANDLE) {
            vulkan_context->resource_builder->destroyBuffer(volume_buffer);
        }

        std::vector<VolumeData> volume_datas;
        uint volume_id = 0;
        for (auto &volume_asset: volume_assets) {
            volume_asset->volume_id = volume_id++;
            volume_datas.push_back(volume_asset->volume_data);
        }
        volume_buffer = vulkan_context->resource_builder->stageMemoryToNewBuffer(
                volume_datas.data(), volume_assets.size() * sizeof(VolumeData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }


    void VolumeManager::writeVolumeBuffer() const {
        vulkan_context->descriptor_allocator->writeBuffer(8, volume_buffer.handle, 0,
                                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }

    void VolumeManager::destroy() {
        if (volume_buffer.handle != VK_NULL_HANDLE)
            vulkan_context->resource_builder->destroyBuffer(volume_buffer);
    }
} // RtEngine