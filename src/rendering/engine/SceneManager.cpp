#include "SceneManager.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>
#include <SceneReader.hpp>
#include <SceneWriter.hpp>

namespace RtEngine {
void SceneManager::createScene(std::string scene_path) {
    QuickTimer timer{"Scene Creation", true};

    if (scene != nullptr) {
        scene_resource_deletion_queue.flush();
        for (auto& material : defaultMaterials)
        {
            material.second->reset();
        }
    }

    SceneReader reader = SceneReader(context);
    scene = reader.readScene(scene_path, defaultMaterials);

    scene_resource_deletion_queue.pushFunction([&]() {
        scene->clearRessources();
    });

    createSceneBuffers();
    createBlas();
    createUniformBuffers();
    bufferUpdateFlags = static_cast<uint8_t>(GEOMETRY_UPDATE) | static_cast<uint8_t>(MATERIAL_UPDATE);
}

void SceneManager::createSceneBuffers() {
    std::vector<std::shared_ptr<MeshAsset>> meshes = scene->getMeshes();
    geometry_manager->createGeometryBuffers(scene->getRootNode());
    scene->material->writeMaterial();
}

void SceneManager::createBlas() {
    QuickTimer timer{"BLAS Build", true};
    VkDevice device = context->device_manager->getDevice();

    uint32_t object_id = 0;
    std::vector<std::shared_ptr<MeshAsset>> meshes = scene->getMeshes();
    for (auto& meshAsset : meshes) {
        meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(device, *context->resource_builder, *context->command_manager, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

        meshAsset->accelerationStructure->addTriangleGeometry(geometry_manager->getVertexBuffer(), geometry_manager->getIndexBuffer(),
            meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
            meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
        meshAsset->accelerationStructure->build();
        meshAsset->geometry_id = object_id++;
    }

    scene_resource_deletion_queue.pushFunction([&]()
    {
        for (auto& meshAsset : scene->getMeshes()) {
            meshAsset->accelerationStructure->destroy();
        }
    });
}

void SceneManager::updateSceneDescriptorSets() {
    if (bufferUpdateFlags & GEOMETRY_UPDATE)
    {
        context->descriptor_allocator->writeBuffer(3, geometry_manager->getVertexBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(4, geometry_manager->getIndexBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(5, geometry_manager->getGeometryMappingBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        std::vector<VkImageView> views{};
        for (uint32_t i = 0; i < 6; i++) {
            views.push_back(scene->environment_map[i].imageView);
        }
        context->descriptor_allocator->writeImages(8, views, defaultSamplerLinear, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        VkDevice device = context->device_manager->getDevice();
        for (int i = 0; i < max_frames_in_flight; i++) {
            context->descriptor_allocator->updateSet(device, scene_descriptor_sets[i]);
            context->descriptor_allocator->clearWrites();
        }
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

    scene_descsriptor_set_layout = layoutBuilder.build(context->device_manager->getDevice(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
    main_deletion_queue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device_manager->getDevice(), scene_descsriptor_set_layout, nullptr);
    });
}

void SceneManager::createSceneDescriptorSets(const VkDescriptorSetLayout& layout)
{
    for (int i = 0; i < max_frames_in_flight; i++) {
        scene_descriptor_sets.push_back(context->descriptor_allocator->allocate(context->device_manager->getDevice(), layout));
    }
}

void SceneManager::createUniformBuffers() {
    VkDeviceSize size = sizeof(SceneData);

    sceneUniformBuffers.resize(max_frames_in_flight);
    sceneUniformBuffersMapped.resize(max_frames_in_flight);

    for (size_t i = 0; i < max_frames_in_flight; i++) {
        sceneUniformBuffers[i] = context->resource_builder->createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(context->device_manager->getDevice(), sceneUniformBuffers[i].bufferMemory, 0, size, 0, &sceneUniformBuffersMapped[i]);

        scene_resource_deletion_queue.pushFunction([&, i]() {
            context->resource_builder->destroyBuffer(sceneUniformBuffers[i]);
        });
    }
}

void SceneManager::updateScene(DrawContext& draw_context, uint32_t current_image_idx, AllocatedImage current_image, AllocatedImage& rng_tex) {
    //QuickTimer timer{"Scene Update", true};
    VkDevice device = context->device_manager->getDevice();

    scene->update(context->swapchain->extent.width, context->swapchain->extent.height);

    if (!(bufferUpdateFlags & NO_UPDATE))
        vkDeviceWaitIdle(device);

    if (top_level_acceleration_structure == nullptr) {
        top_level_acceleration_structure = std::make_shared<AccelerationStructure>(device, *context->resource_builder, *context->command_manager, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

        scene_resource_deletion_queue.pushFunction([&]() {
            top_level_acceleration_structure->destroy();
            top_level_acceleration_structure = nullptr;
        });
    }

    draw_context.objects.clear();
    scene->nodes["root"]->draw(draw_context);

    if (bufferUpdateFlags & GEOMETRY_UPDATE) {
        instance_mapping_buffer = createInstanceMappingBuffer(draw_context.objects);
        scene_resource_deletion_queue.pushFunction([&]() {
            context->resource_builder->destroyBuffer(instance_mapping_buffer);
            instance_mapping_buffer.handle = VK_NULL_HANDLE;
        });
    }

    if (bufferUpdateFlags & GEOMETRY_UPDATE || bufferUpdateFlags & MATERIAL_UPDATE)
    {
        if (emitting_instances_buffer.handle != VK_NULL_HANDLE)
            context->resource_builder->destroyBuffer(emitting_instances_buffer);
        else
        {
            scene_resource_deletion_queue.pushFunction([&]() {
                context->resource_builder->destroyBuffer(getEmittingInstancesBuffer());
                emitting_instances_buffer.handle = VK_NULL_HANDLE;
            });
        }
        emitting_instances_buffer = createEmittingInstancesBuffer(draw_context.objects, getMaterial());
    }

    if (bufferUpdateFlags != NO_UPDATE)
    {
        if (bufferUpdateFlags & MATERIAL_UPDATE)
        {
            scene->material->writeMaterial();
        }
    }

    updateSceneDescriptorSets();

    uint32_t instance_id = 0;
    for (int i = 0; i < draw_context.objects.size(); i++) {
        top_level_acceleration_structure->addInstance(draw_context.objects[i].acceleration_structure, draw_context.objects[i].transform, instance_id++);
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
    context->descriptor_allocator->updateSet(device, scene_descriptor_sets[0]);
    context->descriptor_allocator->clearWrites();

    std::shared_ptr<SceneData> scene_data = scene->createSceneData();
    memcpy(sceneUniformBuffersMapped[current_image_idx], scene_data.get(), sizeof(SceneData));

    bufferUpdateFlags = NO_UPDATE;
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

AllocatedBuffer SceneManager::getEmittingInstancesBuffer()
{
    return emitting_instances_buffer;
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
    VkDevice device = context->device_manager->getDevice();

    VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context->device_manager->getPhysicalDevice(), &properties);
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerAnisotropic) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    main_deletion_queue.pushFunction([&]() {
        vkDestroySampler(context->device_manager->getDevice(), defaultSamplerLinear, nullptr);
        vkDestroySampler(context->device_manager->getDevice(), defaultSamplerNearest, nullptr);
        vkDestroySampler(context->device_manager->getDevice(), defaultSamplerAnisotropic, nullptr);
    });
}

void SceneManager::createDefaultMaterials(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) {
    phong_material = std::make_shared<PhongMaterial>(context);
    phong_material->buildPipelines(scene_descsriptor_set_layout);
    main_deletion_queue.pushFunction([&]() {
        phong_material->clearRessources();
    });
    phong_material->pipeline->createShaderBindingTables(raytracingProperties);
    defaultMaterials["phong"] = phong_material;

    metal_rough_material = std::make_shared<MetalRoughMaterial>(context, defaultSamplerLinear);
    metal_rough_material->buildPipelines(scene_descsriptor_set_layout);
    main_deletion_queue.pushFunction([&]() {
        metal_rough_material->clearRessources();
    });
    metal_rough_material->pipeline->createShaderBindingTables(raytracingProperties);
    defaultMaterials["metal_rough"] = metal_rough_material;
}

std::shared_ptr<Material> SceneManager::getMaterial() {
    return scene->material;
}

uint32_t SceneManager::getEmittingInstancesCount() {
    assert(emitting_instances_buffer.handle != VK_NULL_HANDLE);
    return emitting_instances_count;
}


void SceneManager::clearRessources() {
    scene_resource_deletion_queue.flush();
    main_deletion_queue.flush();
}
}
