#ifndef VULKAN_RAYTRACING_WINDOW_HPP
#define VULKAN_RAYTRACING_WINDOW_HPP
#include "GuiWindow.hpp"
#include <GLFW/glfw3.h>

namespace RtEngine {
    class Window {
    public:
        Window() = default;
        Window(uint32_t width, uint32_t height);

        bool is_open() const;

        static void pollEvents();
        GLFWwindow* getHandle() const;
        void destroy() const;

        void addResizeCallback(const std::function<void(int, int)> &func);
        void addKeyCallback(const std::function<void(int, int, int, int)> &func);
        void addMouseCallback(const std::function<void(double, double)> &func);

        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
        static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void mouseCallback(GLFWwindow *window, double xPos, double yPos);

        std::vector<std::function<void(int, int)>> resize_callbacks;
        std::vector<std::function<void(int, int, int, int)>> key_callbacks;
        std::vector<std::function<void(double, double)>> mouse_callbacks;
    private:
        void initGlfwWindow(uint32_t width, uint32_t height);

        GLFWwindow *glfw_handle;


    };
} // RtEngine

#endif //VULKAN_RAYTRACING_WINDOW_HPP