#include "SceneReader.hpp"

#include <AccelerationStructure.hpp>
#include <InteractiveCamera.hpp>
#include <MeshRenderer.hpp>
#include <MetalRoughMaterial.hpp>
#include <Node.hpp>
#include <QuickTimer.hpp>
#include <Rigidbody.hpp>
#include <TransformUtil.hpp>
#include <YAML_glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
	std::shared_ptr<Scene>
	SceneReader::readScene(const std::string &file_path) {
		QuickTimer quick_timer("Reading scene from file");

		try {
			YAML::Node config = YAML::LoadFile(file_path);
			YAML::Node scene_node = config["scene"];

			auto material_name = scene_node["material_name"].as<std::string>();
			std::shared_ptr<Material> material = runtime_context->material_repository->getMaterial(material_name);
			if (!material)
				throw std::runtime_error("Material " + material_name + " does not exist");
			// TODO remove vulkan context from reader?
			std::shared_ptr<Scene> scene =
					std::make_shared<Scene>(file_path, *vulkan_context->resource_builder, material);
			vulkan_context->layout_manager->addLayout(1, material);

			scene->camera = loadCamera(scene_node["camera"]);
			loadSceneLights(scene_node["lights"], scene);

			for (const auto &mesh_node: scene_node["meshes"]) {
				auto mesh_path = mesh_node["path"].as<std::string>();
				runtime_context->mesh_repository->addMesh(mesh_path);
			}

			for (const auto &texture_node: scene_node["textures"]) {
				// todo handle more types
				TextureType type = PARAMETER;
				if (texture_node["is_normal"] && texture_node["is_normal"].as<bool>())
					type = NORMAL;
				runtime_context->texture_repository->addTexture(texture_node["path"].as<std::string>(), type);
			}

			initializeMaterial(scene_node["materials"], material);

			std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
			scene_graph_node->name = "root";
			auto identity = glm::mat4(1.0f);
			scene_graph_node->transform->setLocalTransform(identity);
			scene_graph_node->children = {};
			for (const auto &yaml_mesh_node: scene_node["nodes"]) {
				scene_graph_node->children.push_back(
						processSceneNodesRecursiv(static_cast<YAML::Node>(yaml_mesh_node), scene));
			}
			scene->addNode(scene_graph_node->name, scene_graph_node);

			return scene;
		} catch (const YAML::Exception &e) {
			throw std::runtime_error(e.what());
		}
	}

	std::shared_ptr<Camera> SceneReader::loadCamera(const YAML::Node &camera_node) const {
		if (camera_node["interactive"].as<bool>()) {
			return std::make_shared<InteractiveCamera>(
					vulkan_context->swapchain->extent.width, vulkan_context->swapchain->extent.height,
					camera_node["fov"].as<float>(), camera_node["position"].as<glm::vec3>(),
					camera_node["view_dir"].as<glm::vec3>());
		} else {
			return std::make_shared<Camera>(vulkan_context->swapchain->extent.width,
											vulkan_context->swapchain->extent.height, camera_node["fov"].as<float>(),
											camera_node["position"].as<glm::vec3>(),
											camera_node["view_dir"].as<glm::vec3>());
		}
	}

	void SceneReader::loadSceneLights(const YAML::Node &lights_node, std::shared_ptr<Scene> &scene) {
		if (lights_node["sun"]) {
			YAML::Node sun_node = lights_node["sun"];
			scene->sun = DirectionalLight(sun_node["direction"].as<glm::vec3>(), sun_node["color"].as<glm::vec3>(),
										  sun_node["intensity"].as<float>());
		}

		uint32_t point_light_index = 0;
		for (auto &point_light_node: lights_node["point_lights"]) {
			scene->pointLights[point_light_index++] =
					PointLight(point_light_node["position"].as<glm::vec3>(), point_light_node["color"].as<glm::vec3>(),
							   point_light_node["intensity"].as<float>());
		}
	}

	void SceneReader::initializeMaterial(const YAML::Node &material_node, std::shared_ptr<Material> &material) {
		runtime_context->curr_material = material;
		if (typeid(*material) == typeid(MetalRoughMaterial)) {
			auto metal_rough_material = dynamic_cast<MetalRoughMaterial *>(material.get());

			for (const auto &material_node: material_node) {
				MetalRoughInstance::Parameters parameters{};

				if (material_node["albedo"])
					parameters.albedo = material_node["albedo"].as<glm::vec3>();
				if (material_node["albedo_tex"])
					parameters.albedo_tex_name = material_node["albedo_tex"].as<std::string>();

				if (material_node["metallic"])
					parameters.metallic = material_node["metallic"].as<float>();
				if (material_node["roughness"])
					parameters.roughness = material_node["roughness"].as<float>();
				if (material_node["ao"])
					parameters.ao = material_node["ao"].as<float>();
				if (material_node["metal_rough_ao_tex"])
					parameters.metal_rough_ao_tex_name = material_node["metal_rough_ao_tex"].as<std::string>();

				if (material_node["eta"])
					parameters.eta = material_node["eta"].as<float>();

				if (material_node["normal_tex"])
					parameters.normal_tex_name = material_node["normal_tex"].as<std::string>();

				if (material_node["emission_power"]) {
					parameters.emission_color = material_node["emission_color"].as<glm::vec3>();
					parameters.emission_power = material_node["emission_power"].as<float>();
				}

				metal_rough_material->createInstance(parameters);
			}
		} else if (typeid(*material) == typeid(PhongMaterial)) {
			auto phong_material = dynamic_cast<PhongMaterial *>(material.get());

			for (const auto &material_node: material_node) {
				glm::vec3 diffuse = glm::vec3(1.0);
				if (material_node["diffuse"])
					diffuse = material_node["diffuse"].as<glm::vec3>();

				glm::vec3 specular = glm::vec3(1.0);
				if (material_node["specular"])
					specular = material_node["specular"].as<glm::vec3>();

				glm::vec3 ambient = glm::vec3(1.0);
				if (material_node["ambient"])
					ambient = material_node["ambient"].as<glm::vec3>();

				glm::vec3 reflection = glm::vec3(0);
				if (material_node["reflection"])
					reflection = material_node["reflection"].as<glm::vec3>();

				glm::vec3 transmission = glm::vec3(0);
				if (material_node["transmission"])
					transmission = material_node["transmission"].as<glm::vec3>();

				float n = 1.0;
				if (material_node["n"])
					n = material_node["n"].as<float>();

				glm::vec3 eta = glm::vec3(1.0);
				if (material_node["eta"])
					eta = material_node["eta"].as<glm::vec3>();

				phong_material->createInstance(diffuse, specular, ambient, reflection, transmission, n, eta);
			}
		}
	}

	std::shared_ptr<Node> SceneReader::processSceneNodesRecursiv(const YAML::Node &yaml_node,
																 const std::shared_ptr<Scene> &scene) {
		std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
		scene_graph_node->name = yaml_node["name"].as<std::string>();
		readComponents(yaml_node, scene_graph_node);
		scene_graph_node->children = {};
		for (auto &child_node: yaml_node["children"]) {
			scene_graph_node->children.push_back(processSceneNodesRecursiv(child_node, scene));
		}
		scene_graph_node->refreshTransform(glm::mat4(1.0f));

		scene->addNode(scene_graph_node->name, scene_graph_node);
		return scene_graph_node;
	}

	void SceneReader::readComponents(const YAML::Node &yaml_node, std::shared_ptr<Node> &scene_node) {
		for (auto &comp_node: yaml_node["components"]) {
			std::string comp_name = "";
			YAML::Node section_node;
			for (auto &pair: comp_node) {
				comp_name = pair.first.as<std::string>();
				section_node = comp_node[comp_name];
			}
			if (comp_name == Transform::COMPONENT_NAME) {
				scene_node->transform->initProperties(comp_node);
			} else if (comp_name == MeshRenderer::COMPONENT_NAME) {
				std::shared_ptr<MeshRenderer> mesh_component =
						std::make_shared<MeshRenderer>(runtime_context, scene_node);
				mesh_component->initProperties(comp_node);
				scene_node->addComponent(mesh_component);
			} else if (comp_name == Rigidbody::COMPONENT_NAME) {
				auto rb = std::make_shared<Rigidbody>(scene_node);
				rb->initProperties(comp_node);
				scene_node->addComponent(rb);
			}
		}
	}
} // namespace RtEngine
