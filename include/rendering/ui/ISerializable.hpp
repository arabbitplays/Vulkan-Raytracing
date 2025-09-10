//
// Created by oschdi on 10.09.25.
//

#ifndef VULKAN_RAYTRACING_ISERIALIZABLE_HPP
#define VULKAN_RAYTRACING_ISERIALIZABLE_HPP
#include <memory>

#include "PropertiesManager.hpp"

namespace RtEngine {
    class ISerializable {
    public:
        virtual ~ISerializable() = default;
        virtual std::shared_ptr<PropertiesSection> getProperties() = 0;
    };
}

#endif //VULKAN_RAYTRACING_ISERIALIZABLE_HPP