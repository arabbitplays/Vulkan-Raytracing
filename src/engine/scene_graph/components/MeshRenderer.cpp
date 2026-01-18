#include "MeshRenderer.hpp"
#include <Node.hpp>
#include <Scene.hpp>

namespace RtEngine {
	void MeshRenderer::OnStart() {
		mesh_asset = context->mesh_repository->getMesh(mesh_asset_name);
		assert(mesh_asset != nullptr);

		std::shared_ptr<Material> material = context->scene_manager->getCurrentMaterial();
		mesh_material = material->getInstanceByName(material_instance_name);
		assert(mesh_material != nullptr);
	}

	void MeshRenderer::OnRender(DrawContext &ctx) {
		auto shared_node = node.lock();
		if (!shared_node) {
			assert(false);
		}

		glm::mat4 nodeMatrix = shared_node->transform->getWorldTransform();

		ctx.addRenderObject(RenderObject{InstanceMappingData{mesh_asset->geometry_id, mesh_material->getMaterialIndex()},
										   mesh_asset->accelerationStructure, nodeMatrix, mesh_asset->triangle_count, mesh_material->getEmissionPower()});
	}

	void MeshRenderer::definePropertySections() {
		assert(properties != nullptr);

		auto mesh_renderer_section = std::make_shared<PropertiesSection>(COMPONENT_NAME);
		mesh_renderer_section->addString("material_name", &material_instance_name,
									  PERSISTENT_PROPERTY_FLAG);
		mesh_renderer_section->addString("mesh", &mesh_asset_name, PERSISTENT_PROPERTY_FLAG);
		properties->addPropertySection(mesh_renderer_section);

		if (mesh_material) {
			properties->addPropertySection(mesh_material->getProperties(), SERIALIZABLE_PROPERTY_FLAG);
		}
	}

	void MeshRenderer::initProperties(const YAML::Node &config_node) {
		mesh_asset_name = config_node[COMPONENT_NAME]["mesh"].as<std::string>();
		material_instance_name = config_node[COMPONENT_NAME]["material_name"].as<std::string>();
	}

} // namespace RtEngine
