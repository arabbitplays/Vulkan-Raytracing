#ifndef PROPERTIESMANAGER_HPP
#define PROPERTIESMANAGER_HPP

#include <ConfigLoader.hpp>
#include <Properties.hpp>
#include <cassert>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace RtEngine {
	constexpr std::string MATERIAL_SECTION_NAME = "Material";
	constexpr std::string RENDERER_SECTION_NAME = "Renderer";

	constexpr uint32_t MAX_PUSH_CONSTANT_SIZE = 16 * sizeof(uint32_t);

	struct PropertiesSection {
		PropertiesSection(std::string name) : section_name(name) {}

		void addBool(const std::string &name, int32_t *var) {
			auto property = std::make_shared<BoolProperty>(name, var);
			bool_properties.push_back(property);
		}

		void addInt(const std::string &name, int32_t *var, uint32_t flags = ALL_PROPERTY_FLAGS, int32_t min = 0,
					int32_t max = -1) {
			auto property = std::make_shared<IntProperty>(name, var, flags, min, max);
			int_properties.push_back(property);
		}

		void addFloat(const std::string &name, float *var, uint32_t flags = ALL_PROPERTY_FLAGS, float min = 0,
					  float max = -1) {
			auto property = std::make_shared<FloatProperty>(name, var, flags, min, max);
			float_properties.push_back(property);
		}

		void addString(const std::string &name, std::string *var, uint32_t flags = ALL_PROPERTY_FLAGS) {
			auto property = std::make_shared<StringProperty>(name, var, flags);
			string_properties.push_back(property);
		}

		void addVector(const std::string &name, glm::vec3 *var, uint32_t flags = ALL_PROPERTY_FLAGS) {
			auto property = std::make_shared<VectorProperty>(name, var, flags);
			vector_properties.push_back(property);
		}

		void addSelection(const std::string &name, std::string *var, std::vector<std::string> selection_options,
						  uint32_t flags = ALL_PROPERTY_FLAGS) {
			assert(!selection_options.empty());
			auto property = std::make_shared<SelectionProperty>(name, var, selection_options, flags);
			selection_properties.push_back(property);
			*var = selection_options.at(0);
		}

		std::string section_name;
		std::vector<std::shared_ptr<BoolProperty>> bool_properties;
		std::vector<std::shared_ptr<IntProperty>> int_properties;
		std::vector<std::shared_ptr<FloatProperty>> float_properties;
		std::vector<std::shared_ptr<StringProperty>> string_properties;
		std::vector<std::shared_ptr<VectorProperty>> vector_properties;
		std::vector<std::shared_ptr<SelectionProperty>> selection_properties;
	};

	class PropertiesManager {
	public:
		PropertiesManager() = default;
		PropertiesManager(const std::string &config_file_path);
		PropertiesManager(const YAML::Node &config);

		void addPropertySection(const std::shared_ptr<PropertiesSection> &properties,
								uint32_t flags = ALL_PROPERTY_FLAGS);
		void *getPushConstants(uint32_t *size);
		std::vector<std::shared_ptr<PropertiesSection>> getSections(uint32_t filter_flags = NONE_PROPERTY_FLAG);
		bool serialize();

		std::unordered_map<std::string, std::shared_ptr<PropertiesSection>> property_sections;
		std::unordered_map<std::string, uint32_t> section_flags;
		int32_t curr_sample_count, samples_per_pixel = 10;

	private:
		void initSectionWithConfig(const std::shared_ptr<PropertiesSection> &properties);
		void updatePushConstants();

		std::shared_ptr<ConfigLoader> config;
		std::vector<int32_t> push_constants;
	};

} // namespace RtEngine
#endif // PROPERTIESMANAGER_HPP
