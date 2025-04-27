#ifndef BASICS_COMMANDMANAGER_HPP
#define BASICS_COMMANDMANAGER_HPP


#include <vulkan/vulkan_core.h>
#include <optional>

namespace RtEngine {
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class CommandManager {
public:
    VkCommandPool commandPool{};

    CommandManager();
    CommandManager(VkDevice device, QueueFamilyIndices queueFamilyIndices);
    void createCommandPool();
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommand(VkCommandBuffer commandBuffer);
    void destroyCommandManager();

private:
    VkDevice device;
    VkQueue graphicsQueue{};
    QueueFamilyIndices queueFamilyIndices;
};


}
#endif //BASICS_COMMANDMANAGER_HPP
