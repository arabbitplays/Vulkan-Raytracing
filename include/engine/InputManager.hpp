#ifndef VULKAN_RAYTRACING_INPUTMANAGER_HPP
#define VULKAN_RAYTRACING_INPUTMANAGER_HPP
#include <unordered_set>

#include "Window.hpp"

namespace RtEngine {
    enum Keycode {
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        SPACE, SHIFT,
        UNKNOWN
    };

    class InputManager {
    public:
        explicit InputManager(std::shared_ptr<Window> window);

        bool getKeyDown(Keycode key) const;
        glm::vec2 getMousePosition() const;
        void reset();

    private:
        void processGlfwKeyEvent(int key, int action);
        void processGlfwMouseEvent(double xPos, double yPos);

        static Keycode glfwToEngineKeycode(int glfw_key);

        std::shared_ptr<Window> window;

        std::unordered_set<Keycode> pressed_keycodes;
        glm::vec2 mouse_pos;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_INPUTMANAGER_HPP