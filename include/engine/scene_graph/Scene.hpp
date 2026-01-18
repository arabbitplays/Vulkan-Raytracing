#ifndef SCENE_HPP
#define SCENE_HPP

#include <MeshAsset.hpp>
#include <MeshAssetBuilder.hpp>
#include <Node.hpp>
#include <PhongMaterial.hpp>
#include <array>
#include <bits/shared_ptr.h>
#include <glm/vec3.hpp>
#include <utility>

#include "EnvironmentMap.hpp"
#include "IScene.hpp"

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
		uint32_t emitting_object_count;
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

	class Scene : public IScene
	{
	public:
		Scene(std::string path, const std::shared_ptr<Material>& material) :
			path(std::move(path)), material(material) {
		}

		virtual ~Scene() = default;

		void addNode(std::string name, std::shared_ptr<Node> node);
		std::shared_ptr<Node> getRootNode();

		void start();

		void update();
		void destroy();

		std::vector<std::shared_ptr<MeshAsset>> getMeshAssets() override;
		std::vector<std::shared_ptr<MaterialInstance>> getMaterialInstances() override;
		void fillDrawContext(const std::shared_ptr<DrawContext> &draw_context) override;
		std::shared_ptr<Material> getMaterial() override;
		std::shared_ptr<EnvironmentMap> getEnvironmentMap() override;
		void *getSceneData(size_t *size, uint32_t emitting_instances_count) override;

		std::string path;

		std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
		std::shared_ptr<EnvironmentMap> environment_map;

		std::shared_ptr<SceneData> last_scene_data;

		DirectionalLight sun;
		std::array<PointLight, POINT_LIGHT_COUNT> pointLights{};

		std::shared_ptr<Material> material;

	private:
		std::shared_ptr<SceneData> createSceneData(uint32_t emitting_object_count);

	};

} // namespace RtEngine
#endif // SCENE_HPP
