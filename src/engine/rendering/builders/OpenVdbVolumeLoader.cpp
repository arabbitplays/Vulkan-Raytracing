#include "OpenVdbVolumeLoader.hpp"
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>


namespace RtEngine {

    std::shared_ptr<Volume> OpenVdbVolumeLoader::loadVolume(const std::filesystem::path &path) {
        auto volume = std::make_shared<Volume>();
        volume->path = path;

        const std::string density_grid_name = "density";

        openvdb::initialize();

        openvdb::io::File file(path);
        file.open();

        openvdb::GridBase::Ptr baseGrid = file.readGrid(density_grid_name);
        file.close();

        if (!baseGrid) {
            throw std::runtime_error("Grid not found: " + density_grid_name);
        }

        auto floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
        if (!floatGrid) {
            throw std::runtime_error("Grid is not FloatGrid");
        }

        openvdb::CoordBBox bbox = floatGrid->evalActiveVoxelBoundingBox();

        openvdb::Coord m_min = bbox.min();
        openvdb::Coord m_max = bbox.max();

        glm::uvec3 vol_size = glm::uvec3(m_max.x() - m_min.x() + 1, m_max.y() - m_min.y() + 1, m_max.z() - m_min.z() + 1);
        volume->size = vol_size;
        volume->densities.reserve(vol_size.x * vol_size.y * vol_size.z);

        float majorant = 0;
        // Dense sampling
        for (int z = 0; z < vol_size.z; ++z) {
            for (int y = 0; y < vol_size.y; ++y) {
                for (int x = 0; x < vol_size.x; ++x) {
                    openvdb::Coord coord(
                        x + m_min.x(),
                        y + m_min.y(),
                        z + m_min.z()
                    );

                    float density = floatGrid->tree().getValue(coord);
                    majorant = std::max(majorant, density);
                    volume->densities.emplace_back(density);
                }
            }
        }
        volume->max_density = majorant;

        return volume;
    }

} // rtEngine