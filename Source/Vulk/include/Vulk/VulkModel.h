#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Vulk.h"
#include "VulkBufferBuilder.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkFrameUBOs.h"
#include "VulkImageView.h"
#include "VulkMaterialTextures.h"
#include "VulkMesh.h"
#include "VulkPipeline.h"

// a model is the minimal set of things needed to render something in a visually interesting way. it consists of a subset of the following:
// * a mesh
// * material textures (normal, diffuse, etc.   )
// * a material (specular, shininess, etc.) - assuming this isn't coming from a texture
// * how these things are bound to the pipeline (the descriptor set info)
// * a set of vertex and index buffers
struct VulkModel {
    Vulk& vk;
    std::shared_ptr<VulkMesh> mesh;
    std::shared_ptr<VulkMaterialTextures> textures;
    std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> materialUBO;
    uint32_t numIndices, numVertices;
    std::unordered_map<vulk::cpp2::VulkShaderLocation, std::shared_ptr<VulkBuffer>> bufs; // each index is VulkShaderLocation_Pos, Color, Normal, etc.;
    std::shared_ptr<VulkBuffer> indexBuf;

    VulkModel(Vulk& vk, std::shared_ptr<VulkMesh> meshIn, std::shared_ptr<VulkMaterialTextures> texturesIn, std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> materialUBO,
              std::vector<vulk::cpp2::VulkShaderLocation> const& inputs)
        : vk(vk)
        , mesh(meshIn)
        , textures(texturesIn)
        , materialUBO(materialUBO)
        , numIndices((uint32_t)meshIn->indices.size())
        , numVertices((uint32_t)meshIn->vertices.size())
        , indexBuf(VulkBufferBuilder(vk)
                       .setSize(sizeof(meshIn->indices[0]) * meshIn->indices.size())
                       .setMem(meshIn->indices.data())
                       .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                       .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                       .build()) {
        for (vulk::cpp2::VulkShaderLocation i : inputs) {
            if (i == vulk::cpp2::VulkShaderLocation::Pos || i == vulk::cpp2::VulkShaderLocation::Normal || i == vulk::cpp2::VulkShaderLocation::Tangent) {
                std::vector<glm::vec3> vec3s;
                vec3s.reserve(meshIn->vertices.size());
                for (auto& v : meshIn->vertices) {
                    switch (i) {
                    case vulk::cpp2::VulkShaderLocation::Pos:
                        vec3s.push_back(v.pos);
                        break;
                    case vulk::cpp2::VulkShaderLocation::Normal:
                        vec3s.push_back(v.normal);
                        break;
                    case vulk::cpp2::VulkShaderLocation::Tangent:
                        vec3s.push_back(v.tangent);
                        break;
                    default:
                        VULK_THROW("Unhandled vulk::cpp2::VulkShaderLocation");
                    }
                }
                bufs[i] = VulkBufferBuilder(vk)
                              .setSize(sizeof(vec3s[0]) * vec3s.size())
                              .setMem(vec3s.data())
                              .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                              .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                              .build();
            } else if (i == vulk::cpp2::VulkShaderLocation::TexCoord) {
                std::vector<glm::vec2> vec2s;
                vec2s.reserve(meshIn->vertices.size());
                for (auto& v : meshIn->vertices) {
                    vec2s.push_back(v.uv);
                }
                bufs[i] = VulkBufferBuilder(vk)
                              .setSize(sizeof(vec2s[0]) * vec2s.size())
                              .setMem(vec2s.data())
                              .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                              .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                              .build();
            } else {
                VULK_THROW("Unhandled vulk::cpp2::VulkShaderLocation");
            }
        }
    }

    void bindInputBuffers(VkCommandBuffer cmdBuf) {
        for (auto& [binding, buf] : this->bufs) {
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuf, (uint32_t)binding, 1, &buf->buf, offsets);
        }

        vkCmdBindIndexBuffer(cmdBuf, indexBuf->buf, 0, VK_INDEX_TYPE_UINT32);
    }
};
