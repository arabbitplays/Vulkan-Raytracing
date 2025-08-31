//
// Created by oschdi on 31.08.25.
//

#ifndef ILAYOUTPROVIDER_HPP
#define ILAYOUTPROVIDER_HPP
#include "DescriptorSet.hpp"

namespace RtEngine {
    class ILayoutProvider {
    public:
        virtual ~ILayoutProvider() = default;

        void initLayout() {
            descriptor_layout = createLayout();
            descriptor_set = createDescriptorSet(descriptor_layout);
            createAndBindResources();
        }

        virtual void createAndBindResources() = 0;
        virtual void destroyResources() = 0;

        virtual VkDescriptorSetLayout getLayout() const {
            return descriptor_layout;
        };
        virtual VkDescriptorSet getDescriptorSet(uint32_t current_frame = 0) const {
            return descriptor_set->getCurrentSet(current_frame);
        };


    protected:
        virtual VkDescriptorSetLayout createLayout() = 0;
        virtual std::shared_ptr<DescriptorSet> createDescriptorSet(const VkDescriptorSetLayout &layout) = 0;

        VkDescriptorSetLayout descriptor_layout = VK_NULL_HANDLE;
        std::shared_ptr<DescriptorSet> descriptor_set;
    };
}



#endif //ILAYOUTPROVIDER_HPP
