#pragma once

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>

#include "Vulk.h"
#include "VulkTextureView.h"
#include "VulkMesh.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkFrameUBOs.h"
#include "VulkBufferBuilder.h"
#include "VulkDescriptorSetBuilder.h"
#include "VulkPipelineBuilder.h"
#include "VulkMaterialTextures.h"

// a model is the minimal set of things needed to render something in a visually interesting way. it consists of:
// * a mesh
// * a set of textures (normal, diffuse, etc.   )
// * a material (specular, shininess, etc.)
// * how these things are bound to the pipeline (the descriptor set info)
// * a set of vertex and index buffers
struct VulkModel
{
    Vulk &vk;
    std::shared_ptr<VulkMesh> mesh;
    std::shared_ptr<VulkMaterialTextures> textures;
    std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> materialUBO;
    uint32_t numIndices;
    VulkBuffer vertBuf, indexBuf;

    VulkModel(Vulk &vk, std::shared_ptr<VulkMesh> meshIn, std::shared_ptr<VulkMaterialTextures> texturesIn, std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> materialUBO)
        : vk(vk),
          mesh(meshIn),
          textures(texturesIn),
          materialUBO(materialUBO),
          numIndices((uint32_t)meshIn->indices.size()),
          vertBuf(VulkBufferBuilder(vk)
                      .setSize(sizeof(meshIn->vertices[0]) * meshIn->vertices.size())
                      .setMem(meshIn->vertices.data())
                      .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                      .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                      .build()),
          indexBuf(VulkBufferBuilder(vk)
                       .setSize(sizeof(meshIn->indices[0]) * meshIn->indices.size())
                       .setMem(meshIn->indices.data())
                       .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                       .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                       .build()) {}
};
