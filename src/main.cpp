#include <exception>
#include <iostream>
#include "rendering/engine/VulkanEngine.hpp"

int main() {
    spdlog::set_level(spdlog::level::debug);

    VulkanEngine app;
    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
