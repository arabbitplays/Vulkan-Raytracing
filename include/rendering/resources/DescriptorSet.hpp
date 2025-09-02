//
// Created by oschdi on 31.08.25.
//

#ifndef DESCRIPTORSET_HPP
#define DESCRIPTORSET_HPP

#include <cassert>
#include <memory>

#include "../../builders/DescriptorAllocator.hpp"
#include "DeviceManager.hpp"

namespace RtEngine {
    class DescriptorSet {
    public:
        DescriptorSet(const std::shared_ptr<DescriptorAllocator> &allocator,
                const std::shared_ptr<DeviceManager> &device_manager,
                const VkDescriptorSetLayout layout,
                const uint32_t frames_in_flight = 1) {
            sets.resize(frames_in_flight);
            for (uint32_t i = 0; i < frames_in_flight; i++) {
                sets[i] = allocator->allocate(device_manager->getDevice(), layout);
            }
        }

        VkDescriptorSet getCurrentSet(uint32_t frame = 0) const {
            assert(frame < sets.size());
            return sets[frame];
        }

    private:
        std::vector<VkDescriptorSet> sets;
    };

}

#endif //DESCRIPTORSET_HPP
