#include "MeshRenderer.hpp"
#include <Node.hpp>
#include <Scene.hpp>

namespace RtEngine {

	void MeshRenderer::OnRender(DrawContext &ctx) {
		auto shared_node = node.lock();
		if (!shared_node) {
			assert(false);
		}

		glm::mat4 nodeMatrix = shared_node->transform->getWorldTransform();

		ctx.addRenderObject(RenderObject{InstanceMappingData{meshAsset->geometry_id, meshMaterial->getMaterialIndex()},
										   meshAsset->accelerationStructure, nodeMatrix, meshAsset->triangle_count, meshMaterial->getEmissionPower()});
	}

	void MeshRenderer::definePropertySections() {
		assert(properties != nullptr);

		auto mesh_renderer_section = std::make_shared<PropertiesSection>(COMPONENT_NAME);
		mesh_renderer_section->addString("material_name", &meshMaterial->name,
									  PERSISTENT_PROPERTY_FLAG);
		mesh_renderer_section->addString("mesh", &meshAsset->name, PERSISTENT_PROPERTY_FLAG);
		properties->addPropertySection(mesh_renderer_section);

		if (meshMaterial) {
			properties->addPropertySection(meshMaterial->getProperties(), SERIALIZABLE_PROPERTY_FLAG);
		}
	}

	void MeshRenderer::initProperties(const YAML::Node &config_node) {
		meshAsset = context->mesh_repository->getMesh(config_node[COMPONENT_NAME]["mesh"].as<std::string>());
		std::shared_ptr<Material> material = context->curr_material.lock();
		assert(material != nullptr);
		meshMaterial = material->getInstanceByName(config_node[COMPONENT_NAME]["material_name"].as<std::string>());
	}

} // namespace RtEngine
