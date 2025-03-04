//
// Created by oschdi on 2/28/25.
//

#ifndef PROPERTIESMANAGER_HPP
#define PROPERTIESMANAGER_HPP

#include <cassert>
#include <ConfigLoader.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

constexpr std::string MATERIAL_SECTION_NAME = "Material";
constexpr std::string RENDERER_SECTION_NAME = "Renderer";

constexpr uint32_t MAX_PUSH_CONSTANT_SIZE = 16 * sizeof(uint32_t);

struct BoolProperty {
    std::string name;
    int32_t* var;
    bool imgui_option;
};

struct IntProperty
{
    std::string name;
    int32_t* var;
    int32_t min, max;
};

struct StringProperty
{
    std::string name;
    std::string* var;
};

struct SelectionProperty
{
    std::string name;
    std::string* var;
    int option_idx;
    std::vector<std::string> selection_options;
};

struct Properties {
public:
    Properties(std::string name) : section_name(name) {}

    void addBool(const std::string& name, int32_t* var) {
        bool_properties.push_back(std::make_shared<BoolProperty>(name, var, *var > 0 ? true : false));
    }

    void addInt(const std::string& name, int32_t* var, int32_t min = 0, int32_t max = -1)
    {
        int_properties.push_back(std::make_shared<IntProperty>(name, var, min, max));
    }

    void addString(const std::string& name, std::string* var)
    {
        string_properties.push_back(std::make_shared<StringProperty>(name, var));
    }

    void addSelection(const std::string& name, std::string* var, std::vector<std::string> selection_options)
    {
        assert(!selection_options.empty());
        selection_properties.push_back(std::make_shared<SelectionProperty>(name, var, 0, selection_options));
        *var = selection_options.at(0);
    }

    void addString(const char* str);

    std::string section_name;
    std::vector<std::shared_ptr<BoolProperty>> bool_properties;
    std::vector<std::shared_ptr<IntProperty>> int_properties;
    std::vector<std::shared_ptr<StringProperty>> string_properties;
    std::vector<std::shared_ptr<SelectionProperty>> selection_properties;
};

class PropertiesManager {
public:
    PropertiesManager(const std::string& config_file_path);

    void addPropertySection(const std::shared_ptr<Properties>& properties);
    void* getPushConstants(uint32_t* size);

    std::unordered_map<std::string, std::shared_ptr<Properties>> properties;
    int32_t curr_sample_count, emitting_instances_count;

private:
    void initSectionWithConfig(const std::shared_ptr<Properties>& properties);
    void updatePushConstants();

    std::shared_ptr<ConfigLoader> config;
    std::vector<int32_t> push_constants;
};



#endif //PROPERTIESMANAGER_HPP
