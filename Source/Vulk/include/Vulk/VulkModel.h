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

// a model is the minimal set of things needed to render something in a visually interesting way. it consists of:
// * a mesh
// * a set of textures (normal, diffuse, etc.   )
// * a material (specular, shininess, etc.)
// * how these things are bound to the pipeline (the descriptor set info)
// * a set of vertex and index buffers
struct VulkModel {
    Vulk &vk;
    std::shared_ptr<VulkMesh> mesh;
    std::shared_ptr<VulkMaterialTextures> textures;
    std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> materialUBO;
    uint32_t numIndices, numVertices;
    uint32_t vertInputMask = VulkShaderLocationMask_None;
    std::unordered_map<VulkShaderLocation, std::shared_ptr<VulkBuffer>> bufs; // each index is VulkShaderLocation_Pos, Color, Normal, etc.;
    std::shared_ptr<VulkBuffer> indexBuf;

    VulkModel(Vulk &vk, std::shared_ptr<VulkMesh> meshIn, std::shared_ptr<VulkMaterialTextures> texturesIn,
              std::shared_ptr<VulkUniformBuffer<VulkMaterialConstants>> materialUBO, std::vector<VulkShaderLocation> const &inputs)
        : vk(vk), mesh(meshIn), textures(texturesIn), materialUBO(materialUBO), numIndices((uint32_t)meshIn->indices.size()),
          numVertices((uint32_t)meshIn->vertices.size()), indexBuf(VulkBufferBuilder(vk)
                                                                       .setSize(sizeof(meshIn->indices[0]) * meshIn->indices.size())
                                                                       .setMem(meshIn->indices.data())
                                                                       .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                                                                       .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                                                                       .build()) {
        for (VulkShaderLocation i : inputs) {
            vertInputMask |= 1 << i;
            if (i == VulkShaderLocation_Pos || i == VulkShaderLocation_Normal || i == VulkShaderLocation_Tangent) {
                std::vector<glm::vec3> vec3s;
                vec3s.reserve(meshIn->vertices.size());
                for (auto &v : meshIn->vertices) {
                    switch (i) {
                    case VulkShaderLocation_Pos:
                        vec3s.push_back(v.pos);
                        break;
                    case VulkShaderLocation_Normal:
                        vec3s.push_back(v.normal);
                        break;
                    case VulkShaderLocation_Tangent:
                        vec3s.push_back(v.tangent);
                        break;
                    default:
                        VULK_THROW("Unhandled VulkShaderLocation");
                    }
                }
                bufs[i] = VulkBufferBuilder(vk)
                              .setSize(sizeof(vec3s[0]) * vec3s.size())
                              .setMem(vec3s.data())
                              .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                              .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                              .build();
            } else if (i == VulkShaderLocation_TexCoord) {
                std::vector<glm::vec2> vec2s;
                vec2s.reserve(meshIn->vertices.size());
                for (auto &v : meshIn->vertices) {
                    vec2s.push_back(v.uv);
                }
                bufs[i] = VulkBufferBuilder(vk)
                              .setSize(sizeof(vec2s[0]) * vec2s.size())
                              .setMem(vec2s.data())
                              .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                              .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                              .build();
            } else {
                VULK_THROW("Unhandled VulkShaderLocation");
            }
        }
    }

    void bindInputBuffers(VkCommandBuffer cmdBuf) {
        for (auto &[binding, buf] : this->bufs) {
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuf, binding, 1, &buf->buf, offsets);
        }

        vkCmdBindIndexBuffer(cmdBuf, indexBuf->buf, 0, VK_INDEX_TYPE_UINT32);
    }
};
