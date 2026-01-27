#ifndef VULKAN_RAYTRACING_UPDATEFLAGS_HPP
#define VULKAN_RAYTRACING_UPDATEFLAGS_HPP


namespace RtEngine {
    enum UpdateFlagValue {
        NO_UPDATE = 0,
        STATIC_GEOMETRY_UPDATE = 1 << 0,
        MATERIAL_UPDATE = 1 << 1,
        SCENE_UPDATE = 1 << 2,
        TARGET_RESET = 1 << 3,
    };

    class UpdateFlags {
    public:
        UpdateFlags() = default;

        void setFlag(UpdateFlagValue flag) {
            flags |= flag;

            if (flag == SCENE_UPDATE) {
                flags |= STATIC_GEOMETRY_UPDATE | MATERIAL_UPDATE | TARGET_RESET;
            }

            if (flag == STATIC_GEOMETRY_UPDATE || flag == MATERIAL_UPDATE) {
                flags |= TARGET_RESET;
            }
        }

        void setFlags(const std::shared_ptr<UpdateFlags> &other) {
            flags |= other->flags;
        }

        bool hasAny() const {
            return flags != 0;
        }

        bool checkFlag(UpdateFlagValue flag) const {
            return (flags & flag) != 0;
        }

        void resetFlags() {
            flags = 0;
        }

        int32_t flags;
    };

    typedef std::shared_ptr<UpdateFlags> UpdateFlagsHandle;
} // RtEngine

#endif //VULKAN_RAYTRACING_UPDATEFLAGS_HPP