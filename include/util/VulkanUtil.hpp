//
// Created by oster on 09.09.2024.
//

#include <string>
#include <vector>
#include <fstream>
#include <vulkan/vulkan_core.h>

#ifndef BASICS_VULKANUTIL_HPP
#define BASICS_VULKANUTIL_HPP

#endif //BASICS_VULKANUTIL_HPP

class VulkanUtil {
    VulkanUtil() = delete;

public:
    static VkShaderModule createShaderModule(VkDevice device, const std::size_t spv_size,
             const uint32_t spv[]) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spv_size;
        createInfo.pCode = spv;

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }
private:
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file " + filename);
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }
};