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

		ctx.objects.push_back(RenderObject{InstanceMappingData{meshAsset->geometry_id, meshMaterial->getMaterialIndex()},
										   meshAsset->accelerationStructure, nodeMatrix, meshAsset->triangle_count, meshMaterial->getEmissionPower()});
	}

	void MeshRenderer::definePropertySections() {
		assert(properties != nullptr);

		// TODO bring this back
		/*auto mesh_renderer_section = std::make_shared<PropertiesSection>(COMPONENT_NAME);
		mesh_renderer_section->addInt("material_idx", reinterpret_cast<int32_t *>(&meshMaterial->material_index),
									  PERSISTENT_PROPERTY_FLAG);
		mesh_renderer_section->addString("mesh", &meshAsset->name, PERSISTENT_PROPERTY_FLAG);
		properties->addPropertySection(mesh_renderer_section);*/

		if (meshMaterial) {
			properties->addPropertySection(meshMaterial->getProperties(), SERIALIZABLE_PROPERTY_FLAG);
		}
	}

	void MeshRenderer::initProperties(const YAML::Node &config_node) {
		meshAsset = context->mesh_repository->getMesh(config_node[COMPONENT_NAME]["mesh"].as<std::string>());
		std::shared_ptr<Material> material = context->curr_material.lock();
		assert(material != nullptr);
		meshMaterial = material->getInstances().at(config_node[COMPONENT_NAME]["material_idx"].as<int>());
	}

} // namespace RtEngine
