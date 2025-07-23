#ifndef SCENE_HPP
#define SCENE_HPP

#include <Camera.hpp>
#include <MeshAsset.hpp>
#include <MeshAssetBuilder.hpp>
#include <Node.hpp>
#include <PhongMaterial.hpp>
#include <array>
#include <bits/shared_ptr.h>
#include <glm/vec3.hpp>
#include <utility>

#define POINT_LIGHT_COUNT 4

namespace RtEngine {
	struct SceneData {
		glm::mat4 inverse_view;
		glm::mat4 inverse_proj;
		glm::vec4 view_pos;
		std::array<glm::vec4, POINT_LIGHT_COUNT> pointLightPositions;
		std::array<glm::vec4, POINT_LIGHT_COUNT> pointLightColors;
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // w for sun power
		glm::vec4 sunlightColor;
	};

	struct PointLight {
		PointLight() = default;
		PointLight(const glm::vec3 &position, const glm::vec3 &color, float intensity) :
			position(position), color(color), intensity(intensity) {}

		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 color = glm::vec3(0.0f);
		float intensity = 0.0f;
	};

	struct DirectionalLight {
		DirectionalLight() = default;
		DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity) :
			direction(direction), color(color), intensity(intensity) {}

		glm::vec3 direction = glm::vec3(0.0f);
		glm::vec3 color = glm::vec3(0.0f);
		float intensity = 0.0f;
	};

	class Scene
	{
	public:
		Scene(std::string  path, ResourceBuilder resource_builder, const std::shared_ptr<Material>& material) :
			path(std::move(path)), resource_builder(std::move(resource_builder)), material(material) {
			Scene::initScene();
		}

		virtual ~Scene() = default;

		void addNode(std::string name, std::shared_ptr<Node> node);
		std::shared_ptr<Node> getRootNode();

		std::shared_ptr<SceneData> createSceneData();
		void update(uint32_t image_width, uint32_t image_height);

		void clearResources();

		std::string path;

		std::shared_ptr<Camera> camera;

		std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
		std::array<AllocatedImage, 6> environment_map{};

		DirectionalLight sun;
		std::array<PointLight, POINT_LIGHT_COUNT> pointLights{};

		ResourceBuilder resource_builder;
		DeletionQueue deletion_queue{};

		std::shared_ptr<Material> material;

	protected:
		void initScene() {
			std::string folder = "textures/cube_map/";
			environment_map[0] = resource_builder.loadTextureImage(folder + "posx.jpg").image;
			environment_map[1] = resource_builder.loadTextureImage(folder + "negx.jpg").image;
			environment_map[2] = resource_builder.loadTextureImage(folder + "posy.jpg").image;
			environment_map[3] = resource_builder.loadTextureImage(folder + "negy.jpg").image;
			environment_map[4] = resource_builder.loadTextureImage(folder + "posz.jpg").image;
			environment_map[5] = resource_builder.loadTextureImage(folder + "negz.jpg").image;

			deletion_queue.pushFunction([&]() {
				for (auto image: environment_map) {
					resource_builder.destroyImage(image);
				}
			});
		};
	};

} // namespace RtEngine
#endif // SCENE_HPP
