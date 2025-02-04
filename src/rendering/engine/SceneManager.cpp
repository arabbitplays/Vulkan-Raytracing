//
// Created by oschdi on 12/30/24.
//

#include "SceneManager.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>
#include <SceneWriter.hpp>

void SceneManager::createScene(SceneType scene_type) {
    QuickTimer timer{"Scene Creation", true};

    if (scene != nullptr) {
        scene_ressource_deletion_queue.flush();
        phong_material->reset();
        metal_rough_material->reset();
    }

    switch (scene_type) {
        case SceneType::PBR_CORNELL_BOX:
            scene = std::make_shared<PBR_CornellBox>(context->mesh_builder, *context->resource_builder, context->swapchain->extent.width, context->swapchain->extent.height, metal_rough_material);
        break;
        case SceneType::CORNELL_BOX:
            scene = std::make_shared<CornellBox>(context->mesh_builder, *context->resource_builder, context->swapchain->extent.width, context->swapchain->extent.height, phong_material);
            break;
        case SceneType::PLANE:
            scene = std::make_shared<PlaneScene>(context->mesh_builder, *context->resource_builder, context->swapchain->extent.width, context->swapchain->extent.height, phong_material);
            break;
        case SceneType::SHOWCASE:
            scene = std::make_shared<Material_Showcase>(context->mesh_builder, *context->resource_builder, context->swapchain->extent.width, context->swapchain->extent.height, metal_rough_material);
        break;
    }
    scene_ressource_deletion_queue.pushFunction([&]() {
        scene->clearRessources();
    });

    createSceneBuffers();
    createBlas();
    createUniformBuffers();
    createSceneDescriptorSets();

    curr_scene_type = scene_type;

    // TODO remove this test
    SceneWriter writer;
    writer.writeScene("test.yaml", scene);
}

void SceneManager::createSceneBuffers() {
    vertex_buffer = createVertexBuffer(scene->meshes);
    index_buffer = createIndexBuffer(scene->meshes);
    scene->material->writeMaterial();

    geometry_mapping_buffer = createGeometryMappingBuffer(scene->meshes);

    scene_ressource_deletion_queue.pushFunction([&]() {
        context->resource_builder->destroyBuffer(vertex_buffer);
        context->resource_builder->destroyBuffer(index_buffer);
        context->resource_builder->destroyBuffer(geometry_mapping_buffer);
    });
}

void SceneManager::createBlas() {
    QuickTimer timer{"BLAS Build", true};
    uint32_t object_id = 0;
    for (auto& meshAsset : scene->meshes) {
        meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(context->device, *context->resource_builder, *context->command_manager, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

        meshAsset->accelerationStructure->addTriangleGeometry(vertex_buffer, index_buffer,
            meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
            meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
        meshAsset->accelerationStructure->build();
        meshAsset->geometry_id = object_id++;
    }
}

void SceneManager::createSceneDescriptorSets() {
    context->descriptor_allocator->writeBuffer(3, vertex_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    context->descriptor_allocator->writeBuffer(4, index_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    context->descriptor_allocator->writeBuffer(5, geometry_mapping_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    std::vector<VkImageView> views{};
    for (uint32_t i = 0; i < 6; i++) {
        views.push_back(scene->environment_map[i].imageView);
    }
    context->descriptor_allocator->writeImages(8, views, defaultSamplerLinear, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    for (int i = 0; i < max_frames_in_flight; i++) {
        scene_descriptor_sets.push_back(context->descriptor_allocator->allocate(context->device, scene_descsriptor_set_layout));
        context->descriptor_allocator->updateSet(context->device, scene_descriptor_sets[i]);
        context->descriptor_allocator->clearWrites();
    }
}

void SceneManager::createSceneLayout() {
    DescriptorLayoutBuilder layoutBuilder;

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR); // TLAS
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render image
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // scene data
    layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // vertex buffer
    layoutBuilder.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // index buffer
    layoutBuilder.addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // geometry buffer
    layoutBuilder.addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // instance buffer
    layoutBuilder.addBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // emitting instances buffer
    layoutBuilder.addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6); // env map
    layoutBuilder.addBinding(9, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // rng tex

    scene_descsriptor_set_layout = layoutBuilder.build(context->device, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
    main_deletion_queue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device, scene_descsriptor_set_layout, nullptr);
    });
}


void SceneManager::createUniformBuffers() {
    VkDeviceSize size = sizeof(SceneData);

    sceneUniformBuffers.resize(max_frames_in_flight);
    sceneUniformBuffersMapped.resize(max_frames_in_flight);

    for (size_t i = 0; i < max_frames_in_flight; i++) {
        sceneUniformBuffers[i] = context->resource_builder->createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(context->device, sceneUniformBuffers[i].bufferMemory, 0, size, 0, &sceneUniformBuffersMapped[i]);

        scene_ressource_deletion_queue.pushFunction([&, i]() {
            context->resource_builder->destroyBuffer(sceneUniformBuffers[i]);
        });
    }
}

void SceneManager::updateScene(DrawContext& draw_context, uint32_t current_image_idx, AllocatedImage& current_image, AllocatedImage& rng_tex) {
    //QuickTimer timer{"Scene Update", true};

    scene->update(context->swapchain->extent.width, context->swapchain->extent.height);

    if (top_level_acceleration_structure == nullptr) {
        top_level_acceleration_structure = std::make_shared<AccelerationStructure>(context->device, *context->resource_builder, *context->command_manager, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

        scene_ressource_deletion_queue.pushFunction([&]() {
            top_level_acceleration_structure->destroy();
            top_level_acceleration_structure = nullptr;
        });
    }

    draw_context.objects.clear();
    for (auto& pair : scene->nodes) {
        pair.second->draw(glm::mat4(1.0f), draw_context);
    }

    // TODO move this to scene creation time
    if (instance_mapping_buffer.handle == VK_NULL_HANDLE) {
        instance_mapping_buffer = createInstanceMappingBuffer(draw_context.objects);
        emitting_instances_buffer = createEmittingInstancesBuffer(draw_context.objects, getMaterial());
        scene_ressource_deletion_queue.pushFunction([&]() {
            context->resource_builder->destroyBuffer(instance_mapping_buffer);
            context->resource_builder->destroyBuffer(emitting_instances_buffer);
            instance_mapping_buffer.handle = VK_NULL_HANDLE;
        });
    }
    uint32_t instance_id = 0;
    for (auto& object : draw_context.objects) {
        top_level_acceleration_structure->addInstance(object.acceleration_structure, object.transform, instance_id++);
    }

    if (top_level_acceleration_structure->getHandle() == VK_NULL_HANDLE) {
        top_level_acceleration_structure->addInstanceGeometry();
    } else {
        top_level_acceleration_structure->update_instance_geometry(0);
    }
    top_level_acceleration_structure->build();

    context->descriptor_allocator->writeAccelerationStructure(0, top_level_acceleration_structure->getHandle(), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    context->descriptor_allocator->writeImage(1, current_image.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    context->descriptor_allocator->writeBuffer(2, sceneUniformBuffers[0].handle, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    context->descriptor_allocator->writeBuffer(6, instance_mapping_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    context->descriptor_allocator->writeBuffer(7, emitting_instances_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    context->descriptor_allocator->writeImage(9, rng_tex.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    context->descriptor_allocator->updateSet(context->device, scene_descriptor_sets[0]);
    context->descriptor_allocator->clearWrites();

    std::shared_ptr<SceneData> scene_data = scene->createSceneData();
    memcpy(sceneUniformBuffersMapped[current_image_idx], scene_data.get(), sizeof(SceneData));
}

AllocatedBuffer SceneManager::createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    VkDeviceSize size =  0;
    for (auto& mesh_asset : mesh_assets) {
        size += mesh_asset->meshBuffers.vertices.size();
    }

    std::vector<Vertex> vertices(size);

    uint32_t vertex_offset = 0;
    for (auto& mesh_asset : mesh_assets) {
        vertices.insert(vertices.begin() + vertex_offset, mesh_asset->meshBuffers.vertices.begin(), mesh_asset->meshBuffers.vertices.end());

        mesh_asset->instance_data.vertex_offset = vertex_offset;
        vertex_offset += mesh_asset->meshBuffers.vertices.size();
    }

    return context->resource_builder->stageMemoryToNewBuffer(vertices.data(), vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer SceneManager::createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    VkDeviceSize size =  0;
    for (auto& mesh_asset : mesh_assets) {
        size += mesh_asset->meshBuffers.indices.size();
    }

    std::vector<uint32_t> indices(size);

    uint32_t index_offset = 0;
    for (auto& mesh_asset : mesh_assets) {
        indices.insert(indices.begin() + index_offset, mesh_asset->meshBuffers.indices.begin(), mesh_asset->meshBuffers.indices.end());

        mesh_asset->instance_data.triangle_offset = index_offset;
        index_offset += mesh_asset->meshBuffers.indices.size();
    }

    return context->resource_builder->stageMemoryToNewBuffer(indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer SceneManager::createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    std::vector<GeometryData> geometry_datas;
    for (auto& mesh_asset : mesh_assets) {
        geometry_datas.push_back(mesh_asset->instance_data);
    }
    return context->resource_builder->stageMemoryToNewBuffer(geometry_datas.data(), mesh_assets.size() * sizeof(GeometryData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer SceneManager::createInstanceMappingBuffer(std::vector<RenderObject>& objects) {
    assert(!objects.empty());

    std::vector<InstanceData> instance_datas;
    for (int i = 0; i < objects.size(); i++) {
        instance_datas.push_back(objects[i].instance_data);
    }

    return context->resource_builder->stageMemoryToNewBuffer(instance_datas.data(), instance_datas.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer SceneManager::createEmittingInstancesBuffer(std::vector<RenderObject>& objects, std::shared_ptr<Material> material) {
    assert(!objects.empty());

    std::vector<EmittingInstanceData> emitting_instances;
    for (int i = 0; i < objects.size(); i++) {
        EmittingInstanceData instance_data;
        instance_data.instance_id = i;
        instance_data.model_matrix = objects[i].transform;
        float power = material->getEmissionForInstance(objects[i].instance_data.material_index).w;
        instance_data.primitive_count = objects[i].primitive_count;
        if (power > 0.0f || (i == objects.size() - 1 && emitting_instances.empty())) {
            emitting_instances.push_back(instance_data);
        }
    }

    emitting_instances_count = emitting_instances.size();
    return context->resource_builder->stageMemoryToNewBuffer(emitting_instances.data(), emitting_instances.size() * sizeof(EmittingInstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void SceneManager::initDefaultResources(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) {
    createDefaultTextures();
    createDefaultSamplers();
    createDefaultMaterials(raytracingProperties);
}

void SceneManager::createDefaultTextures() {
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    whiteImage = context->resource_builder->createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
    greyImage = context->resource_builder->createImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                              VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    blackImage = context->resource_builder->createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    //checkerboard image
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    errorCheckerboardImage = context->resource_builder->createImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    main_deletion_queue.pushFunction([&]() {
        context->resource_builder->destroyImage(whiteImage);
        context->resource_builder->destroyImage(greyImage);
        context->resource_builder->destroyImage(blackImage);
        context->resource_builder->destroyImage(errorCheckerboardImage);
    });
}

void SceneManager::createDefaultSamplers() {
    VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context->physicalDevice, &properties);
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &defaultSamplerAnisotropic) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    main_deletion_queue.pushFunction([&]() {
        vkDestroySampler(context->device, defaultSamplerLinear, nullptr);
        vkDestroySampler(context->device, defaultSamplerNearest, nullptr);
        vkDestroySampler(context->device, defaultSamplerAnisotropic, nullptr);
    });
}

void SceneManager::createDefaultMaterials(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) {
    phong_material = std::make_shared<PhongMaterial>(context);
    phong_material->buildPipelines(scene_descsriptor_set_layout);
    main_deletion_queue.pushFunction([&]() {
        phong_material->clearRessources();
    });
    phong_material->pipeline->createShaderBindingTables(raytracingProperties);

    metal_rough_material = std::make_shared<MetalRoughMaterial>(context, defaultSamplerLinear);
    metal_rough_material->buildPipelines(scene_descsriptor_set_layout);
    main_deletion_queue.pushFunction([&]() {
        metal_rough_material->clearRessources();
    });
    metal_rough_material->pipeline->createShaderBindingTables(raytracingProperties);
}

std::shared_ptr<Material> SceneManager::getMaterial() {
    return scene->material;
}

uint32_t SceneManager::getEmittingInstancesCount() {
    assert(emitting_instances_buffer.handle != VK_NULL_HANDLE);
    return emitting_instances_count;
}


void SceneManager::clearRessources() {
    scene_ressource_deletion_queue.flush();
    main_deletion_queue.flush();
}
