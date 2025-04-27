#include "SceneWriter.hpp"
#include <iostream>
#include <fstream>
#include <MeshRenderer.hpp>
#include <QuickTimer.hpp>
#include <TransformUtil.hpp>
#include <spdlog/spdlog.h>
#include <YAML_glm.hpp>


namespace RtEngine {
void SceneWriter::writeScene(const std::string& filename, std::shared_ptr<Scene> scene) {
    QuickTimer quick_timer("Writing scene to file");

    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "scene" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "material_name" << YAML::Value << scene->material->name;

    out << YAML::Key << "camera" << YAML::Value << YAML::BeginMap;
    Camera camera = *scene->camera;
    out << YAML::Key << "position" << YAML::Value << YAML::convert<glm::vec3>::encode(camera.position);
    out << YAML::Key << "view_dir" << YAML::Value << YAML::convert<glm::vec3>::encode(camera.view_dir);
    out << YAML::Key << "fov" << YAML::Value << camera.fov;
    out << YAML::Key << "interactive" << YAML::Value << (typeid(*scene->camera) == typeid(InteractiveCamera));
    out << YAML::EndMap;

    writeSceneLights(out, scene);

    out << YAML::Key << "meshes" << YAML::Value << YAML::BeginSeq;
    for (const auto& mesh : scene->meshes) {
      out << YAML::BeginMap;
      out << YAML::Key << "name" << YAML::Value << mesh.first;
      out << YAML::Key << "path" << YAML::Value << mesh.second->path;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    /*out << YAML::Key << "textures" << YAML::Value << YAML::BeginSeq;
    for (const auto& texture : scene->textures) {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << texture.first;
        out << YAML::Key << "path" << YAML::Value << texture.second->path;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;*/ // TODO write textures from repo

    writeMaterial(out, scene->material);

    out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
    for (const auto& node : scene->nodes["root"]->children) {
        writeSceneNode(out, node);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream fout(filename);
    fout << out.c_str();
    fout.close();

    spdlog::info("Scene successfully written to {}", filename);
}

void SceneWriter::writeMaterial(YAML::Emitter& out, const std::shared_ptr<Material>& material)
{
    if (typeid(*material) == typeid(MetalRoughMaterial))
    {
        auto metal_rough_material = dynamic_cast<MetalRoughMaterial*>(material.get());

        out << YAML::Key << "materials" << YAML::Value << YAML::BeginSeq;
        for (auto& resources : metal_rough_material->getResources())
        {
            out << YAML::BeginMap;

            // TODO write material
            /*if (resources->albedo_tex.name.empty() && resources->metal_rough_ao_tex.name.empty() && resources->normal_tex.name.empty())
            {
                out << YAML::Key << "albedo" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->albedo);
                out << YAML::Key << "metallic" << YAML::Value << resources->constants->properties.x;
                out << YAML::Key << "roughness" << YAML::Value << resources->constants->properties.y;
                out << YAML::Key << "ao" << YAML::Value << resources->constants->properties.z;
            } else
            {
                out << YAML::Key << "albedo_tex" << YAML::Value << resources->albedo_tex.name;
                out << YAML::Key << "metal_rough_ao_tex" << YAML::Value << resources->metal_rough_ao_tex.name;
                out << YAML::Key << "normal_tex" << YAML::Value << resources->normal_tex.name;
            }

            if (resources->constants->emission.w != 0)
            {
                out << YAML::Key << "emission_color" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->emission);
                out << YAML::Key << "emission_power" << YAML::Value << resources->constants->emission.w;
            }*/

            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    } else if (typeid(*material) == typeid(PhongMaterial))
    {
        auto phong_material = dynamic_cast<PhongMaterial*>(material.get());

        out << YAML::Key << "materials" << YAML::Value << YAML::BeginSeq;
        for (auto& resources : phong_material->getResources())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "diffuse" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->diffuse);
            out << YAML::Key << "specular" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->specular);
            out << YAML::Key << "ambient" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->ambient);
            out << YAML::Key << "reflection" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->reflection);
            out << YAML::Key << "transmission" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->transmission);
            out << YAML::Key << "n" << YAML::Value << resources->constants->n;
            out << YAML::Key << "eta" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->constants->eta);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    } else
    {
        if (material != nullptr)
        {
            std::string material_name = typeid(*material).name();
            spdlog::error("Writing " + material_name + " not suported");
        }
    }
}

void SceneWriter::writeSceneLights(YAML::Emitter& out, const std::shared_ptr<Scene>& scene)
{
    out << YAML::Key << "lights" << YAML::Value << YAML::BeginMap;

    if (scene->sun.intensity != 0)
    {
        out << YAML::Key << "sun" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "direction" << YAML::Value << YAML::convert<glm::vec3>::encode(scene->sun.direction);
        out << YAML::Key << "color" << YAML::Value << YAML::convert<glm::vec3>::encode(scene->sun.color);
        out << YAML::Key << "intensity" << YAML::Value << scene->sun.intensity;
        out << YAML::EndMap;
    }

    out << YAML::Key << "point_lights" << YAML::Value << YAML::BeginSeq;
    for (const auto& light : scene->pointLights) {
        if (light.intensity != 0)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "position" << YAML::Value << YAML::convert<glm::vec3>::encode(light.position);
            out << YAML::Key << "color" << YAML::Value << YAML::convert<glm::vec3>::encode(light.color);
            out << YAML::Key << "intensity" << YAML::Value << light.intensity;
            out << YAML::EndMap;
        }
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
}

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

void SceneWriter::writeSceneNode(YAML::Emitter& out, const std::shared_ptr<Node>& node)
{
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << node->name;

    TransformUtil::DecomposedTransform transform = TransformUtil::decomposeMatrix(node->transform->getLocalTransform());
    out << YAML::Key << "translation" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.translation);
    out << YAML::Key << "rotation" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.rotation);
    out << YAML::Key << "scale" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.scale);

    std::shared_ptr<MeshRenderer> mesh_renderer = node->getComponent<MeshRenderer>();
    if (mesh_renderer)
    {
        out << YAML::Key << "mesh" << YAML::Value << mesh_renderer->meshAsset->name;
        out << YAML::Key << "material_idx" << YAML::Value << mesh_renderer->meshMaterial->material_index;
    }

    out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;
    for (const auto& child : node->children)
    {
        writeSceneNode(out, child);
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}
}

