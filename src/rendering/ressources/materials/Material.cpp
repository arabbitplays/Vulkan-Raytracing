//
// Created by oschdi on 12/17/24.
//

#include "Material.hpp"

void Material::clearRessources() {
    deletionQueue.flush();
}
