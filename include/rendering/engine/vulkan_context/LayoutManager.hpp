//
// Created by oschdi on 01.09.25.
//

#ifndef LAYOUTMANAGER_HPP
#define LAYOUTMANAGER_HPP
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>

#include "../core/ILayoutProvider.hpp"
#include "DescriptorSet.hpp"

namespace RtEngine {
    class LayoutManager {
    public:
        LayoutManager() = default;

        void addLayout(const uint32_t set_id, const std::shared_ptr<ILayoutProvider> &layout_provider) {
            if (set_id >= layout_providers.size()) {
                layout_providers.resize(set_id + 1);
            }

            layout_providers[set_id] = layout_provider;

            spdlog::debug("Decriptor layout added as set " + std::to_string(set_id));
        }

        [[nodiscard]] std::vector<VkDescriptorSetLayout> getLayouts() const {
            std::vector<VkDescriptorSetLayout> result(layout_providers.size());
            for (uint32_t i = 0; i < layout_providers.size(); i++) {
                result[i] = layout_providers[i]->getDescriptorLayout();
            }
            return result;
        }

        [[nodiscard]] std::vector<VkDescriptorSet> getDescriptorSets(uint32_t current_frame) const {
            std::vector<VkDescriptorSet> result(layout_providers.size());
            for (uint32_t i = 0; i < layout_providers.size(); i++) {
                result[i] = layout_providers[i]->getDescriptorSet(current_frame);
            }
            return result;
        }

    private:
        std::vector<std::shared_ptr<ILayoutProvider>> layout_providers{};
    };
}

#endif //LAYOUTMANAGER_HPP
