#ifndef VULKAN_RAYTRACING_ISERIALIZABLE_HPP
#define VULKAN_RAYTRACING_ISERIALIZABLE_HPP
#include <memory>
#include "IProperties.hpp"

namespace RtEngine {
    class ISerializable {
    public:
        virtual ~ISerializable() = default;

        virtual void initProperties(const std::shared_ptr<IProperties> &config) = 0;
    };
}

#endif //VULKAN_RAYTRACING_ISERIALIZABLE_HPP