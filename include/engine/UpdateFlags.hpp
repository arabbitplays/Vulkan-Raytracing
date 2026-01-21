//
// Created by oschdi on 21.01.26.
//

#ifndef VULKAN_RAYTRACING_UPDATEFLAGS_HPP
#define VULKAN_RAYTRACING_UPDATEFLAGS_HPP

namespace RtEngine {
    enum UpdateFlags {
        NO_UPDATE = 0,
        STATIC_GEOMETRY_UPDATE = 1 << 0,
        MATERIAL_UPDATE = 1 << 1,
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_UPDATEFLAGS_HPP