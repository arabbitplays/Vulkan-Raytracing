//
// Created by oschdi on 2/4/25.
//

#include "SceneWriter.hpp"
#include <iostream>
#include <fstream>
#include <MeshNode.hpp>
#include <spdlog/spdlog.h>

// Custom emitter to serialize glm::vec3
namespace YAML {
    template <>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& v) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            return true;
        }
    };
}


void SceneWriter::writeScene(const std::string& filename, std::shared_ptr<Scene> scene) {
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "scene" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "camera" << YAML::Value << YAML::BeginMap;
    Camera camera = *scene->camera;
    out << YAML::Key << "position" << YAML::Value << YAML::convert<glm::vec3>::encode(camera.position);
    out << YAML::Key << "view_dir" << YAML::Value << YAML::convert<glm::vec3>::encode(camera.view_dir);
    out << YAML::Key << "fov" << YAML::Value << camera.fov;
    out << YAML::Key << "interactive" << YAML::Value << false;
    out << YAML::EndMap;

    out << YAML::Key << "meshes" << YAML::Value << YAML::BeginSeq;
    for (const auto& mesh : scene->meshes) {
      out << YAML::BeginMap;
      out << YAML::Key << "name" << YAML::Value << mesh.first;
      out << YAML::Key << "path" << YAML::Value << mesh.second->path;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "textures" << YAML::Value << YAML::BeginSeq;
    for (const auto& texture : scene->textures) {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << texture.first;
        out << YAML::Key << "path" << YAML::Value << texture.second->path;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
    for (const auto& node : scene->nodes) {
        writeSceneNode(out, node.first, node.second);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream fout(filename);
    fout << out.c_str();
    fout.close();

    spdlog::info("Scene successfully written to {}", filename);
}

struct DecomposedTransform
{
    glm::vec3 translation, rotation, scale;
};

float roundToDecimal(float value, int decimalPlaces) {
    float multiplier = std::pow(10.0f, decimalPlaces);
    return std::round(value * multiplier) / multiplier;
}

// Round each component of the glm::vec3
glm::vec3 roundVec3(glm::vec3 v, int decimalPlaces) {
    v.x = roundToDecimal(v.x, decimalPlaces);
    v.y = roundToDecimal(v.y, decimalPlaces);
    v.z = roundToDecimal(v.z, decimalPlaces);
    return v;
}

DecomposedTransform decomposeMatrix(const glm::mat4& transform) {
    // Extract translation (last column of the matrix)
    glm::vec3 translation = glm::vec3(transform[3]);

    // Extract scale factors (length of the columns, excluding the translation part)
    glm::vec3 scale(
        glm::length(glm::vec3(transform[0])),
        glm::length(glm::vec3(transform[1])),
        glm::length(glm::vec3(transform[2]))
    );

    // Extract rotation by normalizing the 3x3 portion of the matrix
    glm::mat3 rotationMatrix = glm::mat3(transform);
    rotationMatrix[0] /= scale.x;
    rotationMatrix[1] /= scale.y;
    rotationMatrix[2] /= scale.z;

    // Convert rotation matrix to quaternion
    glm::quat rotation = glm::quat_cast(rotationMatrix);
    glm::vec3 eulerAngles = glm::eulerAngles(rotation);
    eulerAngles = glm::degrees(eulerAngles);

    DecomposedTransform result(translation, eulerAngles, scale);
    return result;
}

void SceneWriter::writeSceneNode(YAML::Emitter& out, const std::string& node_name, const std::shared_ptr<Node>& node)
{
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << node_name;

    DecomposedTransform transform = decomposeMatrix(node->localTransform);
    out << YAML::Key << "translation" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.translation);
    out << YAML::Key << "rotation" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.rotation);
    out << YAML::Key << "scale" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.scale);

    if (typeid(*node) == typeid(MeshNode) )
    {
        auto mesh_node = dynamic_cast<MeshNode*>(node.get());
        out << YAML::Key << "mesh" << YAML::Value << mesh_node->meshAsset->name;
    }

    out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;
    for (const auto& child : node->children)
    {
        // TODO do recursion
        break;
        writeSceneNode(out, node_name, child);
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

