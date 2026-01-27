#ifndef VULKAN_RAYTRACING_ISERIALIZABLE_HPP
#define VULKAN_RAYTRACING_ISERIALIZABLE_HPP
#include <memory>
#include "IProperties.hpp"
#include "UpdateFlagValue.hpp"

namespace RtEngine {
    class ISerializable;
    typedef std::shared_ptr<ISerializable> SerializableHandle;

    class ISerializable {
    public:
        virtual ~ISerializable() = default;

        virtual void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle& update_flags) = 0;
    };
}

#endif //VULKAN_RAYTRACING_ISERIALIZABLE_HPP