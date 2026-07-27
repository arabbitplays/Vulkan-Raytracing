#include "OpenVdbVolumeLoader.hpp"
#include <algorithm>
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
        double nonzero_sum = 0.0;
        std::vector<float> nonzero_densities;
        nonzero_densities.reserve(vol_size.x * vol_size.y * vol_size.z);
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
                    if (density > 0.0f) {
                        nonzero_sum += density;
                        nonzero_densities.emplace_back(density);
                    }
                    volume->densities.emplace_back(density);
                }
            }
        }
        volume->max_density = majorant;
        if (nonzero_densities.empty()) {
            volume->avg_density = 0.0f;
            volume->median_density = 0.0f;
        } else {
            volume->avg_density = static_cast<float>(nonzero_sum / nonzero_densities.size());
            const size_t mid = nonzero_densities.size() / 2;
            std::nth_element(nonzero_densities.begin(), nonzero_densities.begin() + mid, nonzero_densities.end());
            float median = nonzero_densities[mid];
            if ((nonzero_densities.size() & 1u) == 0u) {
                float lower_max = *std::max_element(nonzero_densities.begin(), nonzero_densities.begin() + mid);
                median = 0.5f * (lower_max + median);
            }
            volume->median_density = median;
        }

        return volume;
    }

} // rtEngine