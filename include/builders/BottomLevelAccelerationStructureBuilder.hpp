//
// Created by oschdi on 12/14/24.
//

#ifndef BOTTOMLEVELACCELERATIONSTRUCTUREBUILDER_HPP
#define BOTTOMLEVELACCELERATIONSTRUCTUREBUILDER_HPP

#include <AccelerationStructureBuilder.hpp>


class BottomLevelAccelerationStructureBuilder : public AccelerationStructureBuilder {
    using AccelerationStructureBuilder::AccelerationStructureBuilder;

public:
    void addTriangleGeometry(const MeshAsset& mesh);
};



#endif //BOTTOMLEVELACCELERATIONSTRUCTUREBUILDER_HPP
