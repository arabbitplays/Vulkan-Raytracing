#include "InputManager.hpp"

namespace RtEngine {
    InputManager::InputManager(std::shared_ptr<Window> window) : window(window) {
        window->addKeyCallback([this](int key, int scancode, int action, int mods) {
                processGlfwKeyEvent(key, action);
            });
        window->addMouseCallback([this](double xPos, double yPos) {
            processGlfwMouseEvent(xPos, yPos);
        });
    }

    bool InputManager::getKeyDown(const Keycode key) const {
        return pressed_keycodes.contains(key);
    }

    glm::vec2 InputManager::getMousePosition() const {
        return mouse_pos;
    }

    void InputManager::reset() {
        pressed_keycodes.clear();
    }

    void InputManager::processGlfwKeyEvent(int key, int action) {
        if (action == GLFW_PRESS) {
            pressed_keycodes.insert(glfwToEngineKeycode(key));
        }
    }

    void InputManager::processGlfwMouseEvent(double xPos, double yPos) {
        mouse_pos.x = xPos;
        mouse_pos.y = yPos;
    }

    Keycode InputManager::glfwToEngineKeycode(int glfw_key) {
        switch (glfw_key) {
            case GLFW_KEY_A: return Keycode::A;
            case GLFW_KEY_B: return Keycode::B;
            case GLFW_KEY_C: return Keycode::C;
            case GLFW_KEY_D: return Keycode::D;
            case GLFW_KEY_E: return Keycode::E;
            case GLFW_KEY_F: return Keycode::F;
            case GLFW_KEY_G: return Keycode::G;
            case GLFW_KEY_H: return Keycode::H;
            case GLFW_KEY_I: return Keycode::I;
            case GLFW_KEY_J: return Keycode::J;
            case GLFW_KEY_K: return Keycode::K;
            case GLFW_KEY_L: return Keycode::L;
            case GLFW_KEY_M: return Keycode::M;
            case GLFW_KEY_N: return Keycode::N;
            case GLFW_KEY_O: return Keycode::O;
            case GLFW_KEY_P: return Keycode::P;
            case GLFW_KEY_Q: return Keycode::Q;
            case GLFW_KEY_R: return Keycode::R;
            case GLFW_KEY_S: return Keycode::S;
            case GLFW_KEY_T: return Keycode::T;
            case GLFW_KEY_U: return Keycode::U;
            case GLFW_KEY_V: return Keycode::V;
            case GLFW_KEY_W: return Keycode::W;
            case GLFW_KEY_X: return Keycode::X;
            case GLFW_KEY_Y: return Keycode::Y;
            case GLFW_KEY_Z: return Keycode::Z;

            case GLFW_KEY_SPACE: return Keycode::SPACE;
            case GLFW_KEY_LEFT_SHIFT: return Keycode::SHIFT;
            case GLFW_KEY_RIGHT_SHIFT: return Keycode::SHIFT;

            default:
                return Keycode::UNKNOWN;
        }
    }
} // RtEngine