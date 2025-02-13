//
// Created by oschdi on 2/13/25.
//

#ifndef SCENEREADER_H
#define SCENEREADER_H

#include <Scene.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

class SceneReader {
public:
    SceneReader() = default;

    void writeScene(const std::string& filename, std::shared_ptr<Scene> scene);
};



#endif //SCENEREADER_H
