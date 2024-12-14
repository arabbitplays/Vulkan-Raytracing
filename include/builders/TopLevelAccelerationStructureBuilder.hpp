//
// Created by oschdi on 12/14/24.
//

#ifndef TOPLEVELACCELERATIONSTRUCTUREBUILDER_HPP
#define TOPLEVELACCELERATIONSTRUCTUREBUILDER_HPP
#include <AccelerationStructureBuilder.hpp>


class TopLevelAccelerationStructureBuilder : public AccelerationStructureBuilder {
    using AccelerationStructureBuilder::AccelerationStructureBuilder;

public:
    void addInstance(AccelerationStructure blas, glm::mat4 transform_matrix);
    void addInstanceGeometry();
    void update_instance_geometry(uint32_t index);

private:
    void fillInstanceBuffer();

    std::vector<VkAccelerationStructureInstanceKHR> instances{};
};



#endif //TOPLEVELACCELERATIONSTRUCTUREBUILDER_HPP
