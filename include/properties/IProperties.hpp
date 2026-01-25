#ifndef VULKAN_RAYTRACING_IPROPERTIES_HPP
#define VULKAN_RAYTRACING_IPROPERTIES_HPP
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace RtEngine {
#define NONE_PROPERTY_FLAG 0u
#define SERIALIZABLE_PROPERTY_FLAG 1u
#define PERSISTENT_PROPERTY_FLAG 2u
#define ALL_PROPERTY_FLAGS (SERIALIZABLE_PROPERTY_FLAG | PERSISTENT_PROPERTY_FLAG)

    class IProperties {
    public:
        virtual ~IProperties() = default;

        virtual bool startChild(const std::string& name) = 0;
        virtual void endChild() = 0;

        virtual bool addBool(const std::string &name, bool *var) = 0;

        virtual bool addInt(const std::string &name, int32_t *var, int32_t min, int32_t max, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;
        virtual bool addInt(const std::string &name, int32_t *var, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;

        virtual bool addUint(const std::string &name, uint32_t *var, uint32_t min, uint32_t max, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;
        virtual bool addUint(const std::string &name, uint32_t *var, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;

        virtual bool addFloat(const std::string &name, float *var, float min, float max, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;
        virtual bool addFloat(const std::string &name, float *var, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;

        virtual bool addString(const std::string &name, std::string *var, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;

        virtual bool addVector(const std::string &name, glm::vec3 *var, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;
        virtual bool addVector(const std::string &name, glm::vec4 *var, uint32_t flags = ALL_PROPERTY_FLAGS) = 0;

        virtual bool addSelection(const std::string &name, std::string *var, std::vector<std::string> selection_options,
                          uint32_t flags = ALL_PROPERTY_FLAGS) = 0;
    };
}


#endif //VULKAN_RAYTRACING_IPROPERTIES_HPP