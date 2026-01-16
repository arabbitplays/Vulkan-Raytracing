#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include <YAML_glm.hpp>
#include <optional>

namespace RtEngine {
	class ConfigLoader {
	public:
		ConfigLoader(std::string file_path) { loadConfig(file_path); }

		ConfigLoader(YAML::Node config) : config(config) {}

		template<typename T>
		std::optional<T> getConfigValue(const std::string &section_name, const std::string &key) {
			if (!config[section_name].IsDefined() || !config[section_name][key].IsDefined())
				return std::nullopt;

			YAML::Node section_node = config[section_name];
			return std::optional<T>(section_node[key].as<T>());
		}

	private:
		void loadConfig(std::string file_path) { config = YAML::LoadFile(file_path); }

		YAML::Node config;
	};

} // namespace RtEngine
#endif // CONFIGLOADER_HPP
