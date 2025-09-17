//
// Created by oschdi on 17.09.25.
//

#include "PhongInstance.hpp"

#include "PhongMaterial.hpp"

namespace RtEngine {
    PhongInstance::PhongInstance(const Parameters &parameters) {
        diffuse = parameters.diffuse;
        specular = parameters.specular;
        ambient = parameters.ambient;

        reflection = parameters.reflection;
        transmission = parameters.transmission;
        n = parameters.n;
        eta = parameters.eta;
    }

    void PhongInstance::attachTo(Material &m) {
        if (auto* mat = dynamic_cast<PhongMaterial*>(&m)) {
            mat->addInstanceToResources(*this);
        } else {
            throw std::runtime_error("Wrong material-instance pair");
        }
    }

    void PhongInstance::initializeProperties() {
        // TODO implement
    }

} // RtEngine