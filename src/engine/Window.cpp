//
// Created by oschdi on 18.01.26.
//

#include "../../include/engine/Window.hpp"

namespace RtEngine {
    Window::Window(const uint32_t width, const uint32_t height) {
        initGlfwWindow(width, height);
    }

    bool Window::is_open() const {
        return !glfwWindowShouldClose(glfw_handle);
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    GLFWwindow * Window::getHandle() const {
        return glfw_handle;
    }

    void Window::destroy() const {
        glfwDestroyWindow(glfw_handle);
		glfwTerminate();
    }

    void Window::initGlfwWindow(const uint32_t width, const uint32_t height) {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHintString(GLFW_WAYLAND_APP_ID, "test");

        glfw_handle = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(glfw_handle, this);
        glfwSetFramebufferSizeCallback(glfw_handle, framebufferResizeCallback);
        glfwSetKeyCallback(glfw_handle, keyCallback);
        glfwSetCursorPosCallback(glfw_handle, mouseCallback);
    }

    void Window::addResizeCallback(const std::function<void(int, int)> &func) {
        resize_callbacks.push_back(func);
    }

    void Window::addKeyCallback(const std::function<void(int, int, int, int)> &func) {
        key_callbacks.push_back(func);
    }

    void Window::addMouseCallback(const std::function<void(double, double)> &func) {
        mouse_callbacks.push_back(func);
    }

    void Window::framebufferResizeCallback(GLFWwindow *glfw_window, int width, int height) {
        auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfw_window));
        for (const auto& func : window->resize_callbacks) {
            func(width, height);
        }
    }

    void Window::keyCallback(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) {
        auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfw_window));
        for (const auto& func : window->key_callbacks) {
            func(key, scancode, action, mods);
        }
    }

    void Window::mouseCallback(GLFWwindow *glfw_window, double xPos, double yPos) {
        auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(glfw_window));
        for (const auto& func : window->mouse_callbacks) {
            func(xPos, yPos);
        }
    }
} // RtEngine