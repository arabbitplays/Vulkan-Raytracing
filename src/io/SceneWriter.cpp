#include "SceneWriter.hpp"
#include <iostream>
#include <fstream>
#include <MeshRenderer.hpp>
#include <QuickTimer.hpp>
#include <SceneUtil.hpp>
#include <TransformUtil.hpp>
#include <spdlog/spdlog.h>
#include <YAML_glm.hpp>


namespace RtEngine
{
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

    std::vector<std::shared_ptr<MeshAsset>> meshes = SceneUtil::collectMeshAssets(scene->getRootNode());
    out << YAML::Key << "meshes" << YAML::Value << YAML::BeginSeq;
    for (const auto& mesh : meshes) {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << mesh->path;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    std::vector<std::shared_ptr<Texture>> textures = scene->material->getTextures();
    out << YAML::Key << "textures" << YAML::Value << YAML::BeginSeq;
    for (const auto& texture : textures) {
        out << YAML::BeginMap;
        out << YAML::Key << "path" << YAML::Value << texture->path;
        out << YAML::Key << "type" << YAML::Value << texture->type;

        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

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

            out << YAML::Key << "albedo" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->albedo);
            out << YAML::Key << "metallic" << YAML::Value << resources->properties.x;
            out << YAML::Key << "roughness" << YAML::Value << resources->properties.y;
            out << YAML::Key << "ao" << YAML::Value << resources->properties.z;

            // TODO write texture names (myb use properties to handle serialization of materials)

            if (resources->emission.w != 0)
            {
                out << YAML::Key << "emission_color" << YAML::Value << YAML::convert<glm::vec3>::encode(resources->emission);
                out << YAML::Key << "emission_power" << YAML::Value << resources->emission.w;
            }

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

    /*TransformUtil::DecomposedTransform transform = TransformUtil::decomposeMatrix(node->transform->getLocalTransform());
    out << YAML::Key << "translation" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.translation);
    out << YAML::Key << "rotation" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.rotation);
    out << YAML::Key << "scale" << YAML::Value << YAML::convert<glm::vec3>::encode(transform.scale);

    std::shared_ptr<MeshRenderer> mesh_renderer = node->getComponent<MeshRenderer>();
    if (mesh_renderer)
    {
        mesh_renderer->getProperties();
        out << YAML::Key << "mesh" << YAML::Value << mesh_renderer->meshAsset->name;
        out << YAML::Key << "material_idx" << YAML::Value << mesh_renderer->meshMaterial->material_index;
    }*/

    out << YAML::Key << "components" << YAML::Value << YAML::BeginSeq;
    for (auto& component : node->components)
    {
        writeComponent(out, component);
    }
    out << YAML::EndSeq;

    out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;
    for (const auto& child : node->children)
    {
        writeSceneNode(out, child);
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

void SceneWriter::writeComponent(YAML::Emitter& out, const std::shared_ptr<Component>& component)
{
    std::shared_ptr<PropertiesManager> properties = component->getProperties();
    std::vector<std::shared_ptr<PropertiesSection>> sections = properties->getSections(PERSISTENT_PROPERTY_FLAG);
    assert(sections.size() <= 1);
    if (sections.empty())
        return;
    auto section = sections.at(0);

    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << section->section_name;

    for (auto& prop : section->bool_properties)
    {
        if (prop->flags & PERSISTENT_PROPERTY_FLAG)
            out << YAML::Key << prop->name << YAML::Value << *prop->var;
    }

    for (auto& prop : section->float_properties)
    {
        if (prop->flags & PERSISTENT_PROPERTY_FLAG)
            out << YAML::Key << prop->name << YAML::Value << *prop->var;
    }

    for (auto& prop : section->int_properties)
    {
        if (prop->flags & PERSISTENT_PROPERTY_FLAG)
            out << YAML::Key << prop->name << YAML::Value << *prop->var;
    }

    for (auto& prop : section->string_properties)
    {
        if (prop->flags & PERSISTENT_PROPERTY_FLAG)
            out << YAML::Key << prop->name << YAML::Value << *prop->var;
    }

    for (auto& prop : section->vector_properties)
    {
        if (prop->flags & PERSISTENT_PROPERTY_FLAG)
            out << YAML::Key << prop->name << YAML::Value << YAML::convert<glm::vec3>::encode(*prop->var);
    }

    for (auto& prop : section->selection_properties)
    {
        if (prop->flags & PERSISTENT_PROPERTY_FLAG)
            out << YAML::Key << prop->name << YAML::Value << *prop->var;
    }

    out << YAML::EndMap;
}
}

