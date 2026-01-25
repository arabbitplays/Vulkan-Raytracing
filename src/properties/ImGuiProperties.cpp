//
// Created by oschdi on 25.01.26.
//

#include "../../include/properties/ImGuiProperties.hpp"

#include <algorithm>

#include "imgui.h"

namespace RtEngine {
    bool ImGuiProperties::startChild(const std::string &name) {
        return ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    }

    void ImGuiProperties::endChild() {
        ImGui::TreePop();
    }

    bool ImGuiProperties::addBool(const std::string &name, bool *var) {
        return ImGui::Checkbox(name.c_str(), var);
    }

    bool ImGuiProperties::addInt(const std::string &name, int32_t *var, int32_t min, int32_t max, uint32_t flags) {
        return ImGui::SliderInt(name.c_str(), var, min, max);
    }

    bool ImGuiProperties::addInt(const std::string &name, int32_t *var, uint32_t flags) {
        return ImGui::InputInt(name.c_str(), var);
    }

    bool ImGuiProperties::addUint(const std::string &name, uint32_t *var, uint32_t min, uint32_t max, uint32_t flags) {
        return ImGui::SliderScalar(name.c_str(), ImGuiDataType_U32, &var, &min, &max);
    }

    bool ImGuiProperties::addUint(const std::string &name, uint32_t *var, uint32_t flags) {
        return ImGui::DragScalar(name.c_str(), ImGuiDataType_U32, var);
    }

    bool ImGuiProperties::addFloat(const std::string &name, float *var, float min, float max, uint32_t flags) {
        return ImGui::SliderFloat(name.c_str(), var, min, max);
    }

    bool ImGuiProperties::addFloat(const std::string &name, float *var, uint32_t flags) {
        return ImGui::InputFloat(name.c_str(), var);
    }

    bool ImGuiProperties::addString(const std::string &name, std::string *var, uint32_t flags) {
        char* buffer = new char[256];
        bool changed = ImGui::InputText(name.c_str(), buffer, 256, ImGuiInputTextFlags_EnterReturnsTrue);
        if (changed) {
            *var = changed;
        }
        return changed;
    }

    bool ImGuiProperties::addVector(const std::string &name, glm::vec3 *var, uint32_t flags) {
        return ImGui::InputFloat3(name.c_str(), &var->x);
    }

    bool ImGuiProperties::addVector(const std::string &name, glm::vec4 *var, uint32_t flags) {
        return ImGui::InputFloat4(name.c_str(), &var->x);
    }

    bool ImGuiProperties::addSelection(const std::string &name, std::string *var,
        std::vector<std::string> options, uint32_t flags) {
        assert(!options.empty());

        std::vector<const char*> options_c_str;
        std::ranges::transform(options, std::back_inserter(options_c_str),
                               [](auto& str) { return str.c_str(); });

        if (var->empty()) {
            *var = options[0];
        }
        int32_t selected = 0;
        for (int32_t i = 0; i < options.size(); i++) {
            if (options[i] == *var) {
                selected = i;
            }
        }
        bool changed = ImGui::ListBox(name.c_str(), &selected, options_c_str.data(), options_c_str.size());

        *var = options[selected];
        return changed;
    }
} // RtEngine