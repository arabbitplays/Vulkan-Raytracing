#ifndef PROPERTIESMANAGER_HPP
#define PROPERTIESMANAGER_HPP

#include <cassert>
#include <ConfigLoader.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/vec3.hpp>

namespace RtEngine {
constexpr std::string MATERIAL_SECTION_NAME = "Material";
constexpr std::string RENDERER_SECTION_NAME = "Renderer";

constexpr uint32_t MAX_PUSH_CONSTANT_SIZE = 16 * sizeof(uint32_t);

#define SERIALIZABLE_FLAG 0u
#define PERSISTENT_FLAG 1u

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

struct FloatProperty
{
    std::string name;
    float* var;
    float min, max;
};

struct StringProperty
{
    std::string name;
    std::string* var;
};

struct VectorProperty
{
    std::string name;
    glm::vec3* var;
};


struct SelectionProperty
{
    std::string name;
    std::string* var;
    int option_idx;
    std::vector<std::string> selection_options;
};

struct PropertiesSection {
public:
    PropertiesSection(std::string name) : section_name(name) {}

    void addBool(const std::string& name, int32_t* var) {
        bool_properties.push_back(std::make_shared<BoolProperty>(name, var, *var > 0 ? true : false));
    }

    void addInt(const std::string& name, int32_t* var, int32_t min = 0, int32_t max = -1)
    {
        int_properties.push_back(std::make_shared<IntProperty>(name, var, min, max));
    }

    void addFloat(const std::string& name, float* var, float min = 0, float max = -1)
    {
        float_properties.push_back(std::make_shared<FloatProperty>(name, var, min, max));
    }

    void addString(const std::string& name, std::string* var)
    {
        string_properties.push_back(std::make_shared<StringProperty>(name, var));
    }

    void addVector(const std::string& name, glm::vec3* var)
    {
        vector_properties.push_back(std::make_shared<VectorProperty>(name, var));
    }

    void addSelection(const std::string& name, std::string* var, std::vector<std::string> selection_options)
    {
        assert(!selection_options.empty());
        selection_properties.push_back(std::make_shared<SelectionProperty>(name, var, 0, selection_options));
        *var = selection_options.at(0);
    }

    void addString(const char* str);

    std::string section_name;
    uint32_t section_flags;
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
    PropertiesManager(const std::string& config_file_path);

    void addPropertySection(const std::shared_ptr<PropertiesSection>& properties);
    void* getPushConstants(uint32_t* size);
    bool serialize();

    std::unordered_map<std::string, std::shared_ptr<PropertiesSection>> properties;
    int32_t curr_sample_count, emitting_instances_count, samples_per_pixel = 1;

private:
    void initSectionWithConfig(const std::shared_ptr<PropertiesSection>& properties);
    void updatePushConstants();

    std::shared_ptr<ConfigLoader> config;
    std::vector<int32_t> push_constants;
};



}
#endif //PROPERTIESMANAGER_HPP
