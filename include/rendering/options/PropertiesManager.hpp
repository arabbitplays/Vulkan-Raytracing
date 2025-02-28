//
// Created by oschdi on 2/28/25.
//

#ifndef PROPERTIESMANAGER_HPP
#define PROPERTIESMANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

constexpr uint32_t MAX_PUSH_CONSTANT_SIZE = 16 * sizeof(uint32_t);

struct BoolOption {
    std::string name;
    int32_t* var;
    bool imgui_option;
};

struct Properties {
public:
    Properties(std::string name) : section_name(name) {}

    void addBool(const std::string& name, int32_t* var) {
        bool_options.push_back(std::make_shared<BoolOption>(name, var, *var > 0 ? true : false));
    }

    std::string section_name;
    std::vector<std::shared_ptr<BoolOption>> bool_options;
};

class PropertiesManager {
public:
    void addProperties(const std::shared_ptr<Properties>& properties);
    void* getPushConstants(uint32_t* size);

    std::unordered_map<std::string, std::shared_ptr<Properties>> properties;

    int32_t curr_sample_count, emitting_instances_count;
private:
    std::vector<int32_t> push_constants;

    void updatePushConstants();
};



#endif //PROPERTIESMANAGER_HPP
