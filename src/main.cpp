#include <exception>
#include <iostream>
#include "rendering/engine/VulkanEngine.hpp"

int main() {
    VulkanEngine app;
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
