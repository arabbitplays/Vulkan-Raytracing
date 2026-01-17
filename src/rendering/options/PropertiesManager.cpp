#include "PropertiesManager.hpp"

#include <BaseOptions.hpp>
#include <Material.hpp>
#include <ReferenceRenderer.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
	PropertiesManager::PropertiesManager(const std::string &config_file_path) {
		config = std::make_shared<ConfigLoader>(config_file_path);
	}

	PropertiesManager::PropertiesManager(const YAML::Node &config_node) {
		config = std::make_shared<ConfigLoader>(config_node);
	}

	void PropertiesManager::addPropertySection(const std::shared_ptr<PropertiesSection> &section, uint32_t flags) {
		if (section == nullptr)
			return;
		property_sections[section->section_name] = section;
		section_flags[section->section_name] = flags;
		if (config && flags & PERSISTENT_PROPERTY_FLAG)
			initSectionWithConfig(section);
	}

	void PropertiesManager::initSectionWithConfig(const std::shared_ptr<PropertiesSection> &section) {
		for (auto &bool_prop: section->bool_properties) {
			if (!(bool_prop->flags & PERSISTENT_PROPERTY_FLAG))
				continue;
			std::optional<bool> config_value = config->getConfigValue<bool>(section->section_name, bool_prop->name);
			if (config_value.has_value()) {
				*bool_prop->var = config_value.value() ? 1 : 0;
				bool_prop->imgui_option = config_value.value();
			}
		}

		for (auto &int_prop: section->int_properties) {
			if (!(int_prop->flags & PERSISTENT_PROPERTY_FLAG))
				continue;
			std::optional<int32_t> config_value =
					config->getConfigValue<int32_t>(section->section_name, int_prop->name);
			if (config_value.has_value()) {
				*int_prop->var = config_value.value();
			}
		}

		for (auto &float_prop: section->float_properties) {
			if (!(float_prop->flags & PERSISTENT_PROPERTY_FLAG))
				continue;
			std::optional<float> config_value = config->getConfigValue<float>(section->section_name, float_prop->name);
			if (config_value.has_value()) {
				*float_prop->var = config_value.value();
			}
		}

		for (auto &string_prop: section->string_properties) {
			if (!(string_prop->flags & PERSISTENT_PROPERTY_FLAG))
				continue;
			std::optional<std::string> config_value =
					config->getConfigValue<std::string>(section->section_name, string_prop->name);
			if (config_value.has_value()) {
				*string_prop->var = config_value.value();
			}
		}

		for (auto &vector_prop: section->vector_properties) {
			if (!(vector_prop->flags & PERSISTENT_PROPERTY_FLAG))
				continue;
			std::optional<glm::vec3> config_value =
					config->getConfigValue<glm::vec3>(section->section_name, vector_prop->name);
			if (config_value.has_value()) {
				*vector_prop->var = config_value.value();
			}
		}

		for (auto &selection_prop: section->selection_properties) {
			if (!(selection_prop->flags & PERSISTENT_PROPERTY_FLAG))
				continue;
			std::optional<std::string> config_value =
					config->getConfigValue<std::string>(section->section_name, selection_prop->name);
			if (config_value.has_value()) {
				bool valid_value = false;
				for (int32_t i = 0; i < selection_prop->selection_options.size(); i++) {
					if (selection_prop->selection_options[i] == config_value.value()) {
						*selection_prop->var = config_value.value();
						selection_prop->option_idx = i;
						valid_value = true;
						break;
					}
				}

				if (!valid_value)
					spdlog::warn("{} not a valid option for property {} in section", config_value.value(),
								 selection_prop->name, section->section_name);
			}
		}
	}

	void *PropertiesManager::getPushConstants(uint32_t *size, std::shared_ptr<RenderTarget> render_target) {
		updatePushConstants(render_target);
		*size = sizeof(uint32_t) * push_constants.size();
		assert(*size <= MAX_PUSH_CONSTANT_SIZE);
		return push_constants.data();
	}

	int32_t getRecursionDepth(std::vector<std::shared_ptr<IntProperty>> &int_options) {
		for (auto &int_option: int_options) {
			if (int_option->name == RECURSION_DEPTH_OPTION_NAME) {
				return *int_option->var;
			}
		}

		return 0;
	}

	void PropertiesManager::updatePushConstants(std::shared_ptr<RenderTarget> render_target) {
		push_constants.clear();
		assert(property_sections.contains(MATERIAL_SECTION_NAME) && property_sections.contains(RENDERER_SECTION_NAME));

		push_constants.push_back(getRecursionDepth(property_sections[RENDERER_SECTION_NAME]->int_properties));
		std::shared_ptr<PropertiesSection> material_props = property_sections[MATERIAL_SECTION_NAME];
		for (auto &bool_option: material_props->bool_properties) {
			push_constants.push_back(*bool_option->var);
		}

		push_constants.push_back(render_target->getAccumulatedFrameCount());
		push_constants.push_back(render_target->getSamplesPerFrame());
	}

	bool PropertiesManager::serialize() {
		bool change_detected = false;

		for (auto &section: property_sections) {
			if (!(section_flags[section.first] & SERIALIZABLE_PROPERTY_FLAG))
				continue;

			if (ImGui::CollapsingHeader(section.second->section_name.c_str())) {
				for (auto &bool_property: section.second->bool_properties) {
					if (!(bool_property->flags & SERIALIZABLE_PROPERTY_FLAG))
						continue;

					if (ImGui::Checkbox(bool_property->name.c_str(), &bool_property->imgui_option)) {
						*bool_property->var = bool_property->imgui_option ? 1 : 0;
						change_detected = true;
					}
				}

				for (auto &int_property: section.second->int_properties) {
					if (!(int_property->flags & SERIALIZABLE_PROPERTY_FLAG))
						continue;

					if (int_property->max > int_property->min) {
						change_detected |= ImGui::SliderInt(int_property->name.c_str(), int_property->var,
															int_property->min, int_property->max);
					} else {
						change_detected |= ImGui::InputInt(int_property->name.c_str(), int_property->var);
					}
				}

				for (auto &float_property: section.second->float_properties) {
					if (!(float_property->flags & SERIALIZABLE_PROPERTY_FLAG))
						continue;

					if (float_property->max > float_property->min) {
						change_detected |= ImGui::SliderFloat(float_property->name.c_str(), float_property->var,
															  float_property->min, float_property->max);
					} else {
						change_detected |= ImGui::InputFloat(float_property->name.c_str(), float_property->var);
					}
				}

				for (auto &vector_property: section.second->vector_properties) {
					if (!(vector_property->flags & SERIALIZABLE_PROPERTY_FLAG))
						continue;

					change_detected |= ImGui::InputFloat3(vector_property->name.c_str(), &(*vector_property->var)[0]);
				}

				for (auto &selection_property: section.second->selection_properties) {
					if (!(selection_property->flags & SERIALIZABLE_PROPERTY_FLAG))
						continue;

					std::vector<const char *> cStrArray;
					for (auto &str: selection_property->selection_options) {
						cStrArray.push_back(const_cast<char *>(str.c_str())); // Convert std::string to char*
					}
					const char **items = cStrArray.data();

					if (ImGui::Combo(selection_property->name.c_str(), &selection_property->option_idx, items,
									 static_cast<int>(selection_property->selection_options.size()))) {
						*selection_property->var =
								selection_property->selection_options.at(selection_property->option_idx);
						change_detected = true;
					}
				}
			}
		}

		return change_detected;
	}

	std::vector<std::shared_ptr<PropertiesSection>> PropertiesManager::getSections(uint32_t filter_flags) {
		std::vector<std::shared_ptr<PropertiesSection>> result;
		for (auto &section: property_sections) {
			if (filter_flags == 0 || section_flags[section.first] & filter_flags) {
				result.push_back(section.second);
			}
		}
		return result;
	}

} // namespace RtEngine
