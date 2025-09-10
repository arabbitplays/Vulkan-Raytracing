//
// Created by oschdi on 08.09.25.
//

#ifndef VULKAN_RAYTRACING_MATERIALRESOURCEMANAGER_HPP
#define VULKAN_RAYTRACING_MATERIALRESOURCEMANAGER_HPP

#include "ResourceBuilder.hpp"
#include "VulkanContext.hpp"


namespace RtEngine {
    struct MaterialResources {

    };

    template <typename T>
    concept ConcreteResources = std::is_base_of_v<MaterialResources, T>;

    template<ConcreteResources Resources>
    class MaterialResourceManager {
        struct TextureBinding {
            std::vector<std::shared_ptr<Texture>> textures{};
            std::vector<VkImageView> image_views{};
            VkSampler sampler;
        };

    public:
        static constexpr uint32_t MAX_TEXTURE_COUNT = 16;

        MaterialResourceManager() = default;
        explicit MaterialResourceManager(const std::shared_ptr<VulkanContext> &vulkan_context) : vulkan_context(vulkan_context) { }

        // returns the index into the material buffer
        uint32_t addResources(const std::shared_ptr<Resources> &resources) {
            material_resources.push_back(resources);
            return material_resources.size() - 1;
        }

        void addTextureBinding(uint32_t binding_idx, VkSampler& sampler) {
            if (texture_bindings.contains(binding_idx)) {
                spdlog::warn("Texture binding {} already exists!", binding_idx);
                return;
            }

            texture_bindings[binding_idx] = std::make_shared<TextureBinding>();
            texture_bindings[binding_idx]->sampler = sampler;
        }

        uint32_t addTexture(uint32_t binding_idx, std::shared_ptr<Texture> texture) {
            if (!texture_bindings.contains(binding_idx)) {
                spdlog::error("Texture '{}' binding to non existent binding {}", texture->name, binding_idx);
                throw std::runtime_error("Binding doesn't exist!");
            }

            std::shared_ptr<TextureBinding> binding = texture_bindings[binding_idx];
            for (uint32_t i = 0; i < binding->textures.size(); i++) {
                if (binding->textures[i]->name == texture->name) {
                    return i;
                }
            }

            if (binding->textures.size() == MAX_TEXTURE_COUNT) {
                spdlog::error("Texture binding {} is full!", binding_idx);
                return 0;
            }

            binding->textures.push_back(texture);
            binding->image_views.push_back(texture->image.imageView);
            return binding->textures.size() - 1;
        }

        std::vector<std::shared_ptr<Resources>> getResources() {
            return material_resources;
        }

        std::vector<std::shared_ptr<Texture>> getAllTextures() {
            auto append = [](std::vector<std::shared_ptr<Texture>> &dest,
                                     const std::vector<std::shared_ptr<Texture>> &src) {
                for (const auto &elem: src) {
                    if (elem->path.empty())
                        continue;
                    dest.push_back(elem);
                }
            };

            std::vector<std::shared_ptr<Texture>> result(0);

            for (auto& binding : texture_bindings) {
                append(result, binding.second->textures);
            }

            return result;
        }

        void writeResources(DescriptorAllocator& descriptor_allocator, std::shared_ptr<DescriptorSet> descriptor_set) {
            descriptor_allocator.writeBuffer(0, getMaterialBuffer()->handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            for (auto& [binding_idx, binding] : texture_bindings) {
                std::vector<VkImageView> imageViews = getImageViewsForBinding(binding);

                // TODO make this clean so that holes get filled with the default tex
                while (imageViews.size() < MAX_TEXTURE_COUNT) {
                    imageViews.push_back(imageViews[0]);
                }

                descriptor_allocator.writeImages(binding_idx, imageViews, binding->sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }

            VkDevice device = vulkan_context->device_manager->getDevice();
            descriptor_allocator.updateSet(device, descriptor_set->getCurrentSet());
            descriptor_allocator.clearWrites();
        }

        void destroyResources() const {
            if (material_buffer != nullptr)
                vulkan_context->resource_builder->destroyBuffer(*material_buffer);
        }

        void reset() {
            material_resources.clear();
        };

    private:
        std::shared_ptr<AllocatedBuffer> getMaterialBuffer() {
            // TODO Optimization recreate only when needed
            if (material_buffer != nullptr) {
                vulkan_context->resource_builder->destroyBuffer(*material_buffer);
            }

            createMaterialBuffer();
            return material_buffer;
        }

        void createMaterialBuffer() {
            std::vector<Resources> material_data{};
            for (const auto & resources : material_resources) {
                material_data.push_back(*resources);
            }
            material_buffer = std::make_shared<AllocatedBuffer>(
                vulkan_context->resource_builder->stageMemoryToNewBuffer(
                    material_data.data(), material_data.size() * sizeof(Resources),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            );
        }

        std::vector<VkImageView> getImageViewsForBinding(std::shared_ptr<TextureBinding> binding) {
            return binding->image_views;
        }

        std::shared_ptr<VulkanContext> vulkan_context;

        std::shared_ptr<AllocatedBuffer> material_buffer{};
        std::unordered_map<uint32_t, std::shared_ptr<TextureBinding>> texture_bindings{};
        std::vector<std::shared_ptr<Resources>> material_resources{};
    };
}



#endif //VULKAN_RAYTRACING_MATERIALRESOURCEMANAGER_HPP