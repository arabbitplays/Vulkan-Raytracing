//
// Created by oschdi on 3/4/25.
//

#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include <optional>
#include <yaml-cpp/yaml.h>

class ConfigLoader {
public:
    ConfigLoader(std::string file_path)
    {
        loadConfig(file_path);
    }

    template<typename T>
    std::optional<T> getConfigValue(const std::string& section_name, const std::string& key)
    {
        if (!config[section_name].IsDefined() || !config[section_name][key].IsDefined())
            return std::nullopt;

        YAML::Node section_node = config[section_name];
        return std::optional<T>(section_node[key].as<T>());
    }

private:
    void loadConfig(std::string file_path)
    {
        config = YAML::LoadFile(file_path);
    }

    YAML::Node config;
};



#endif //CONFIGLOADER_HPP
