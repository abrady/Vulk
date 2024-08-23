#include "Vulk/VulkMesh.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <iostream>
#include <vector>

glm::vec3 toVec3(const aiVector3D& aiVec) {
    return glm::vec3(aiVec.x, aiVec.y, aiVec.z);
}

void loadModel(char const* model_path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    Assimp::Importer importer;
    unsigned int flags   = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
    const aiScene* scene = importer.ReadFile(model_path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        VULK_THROW("Error loading model");
    }

    // Assuming the model consists of only one mesh. You might need to loop through scene->mNumMeshes for multiple meshes.
    aiMesh* mesh = scene->mMeshes[0];
    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.pos     = toVec3(mesh->mVertices[i]);
        vertex.normal  = toVec3(mesh->mNormals[i]);
        vertex.tangent = toVec3(mesh->mTangents[i]);

        if (mesh->mTextureCoords[0]) {
            // Assimp allows up to 8 texture coordinate sets; we're using the first set here.
            vertex.uv.x = mesh->mTextureCoords[0][i].x;
            vertex.uv.y = mesh->mTextureCoords[0][i].y;
            // vertex.uv.z = mesh->mTextureCoords[0][i].z;
        } else {
            vertex.uv = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
}

namespace std {
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               //    (hash<glm::vec3>()(vertex.tangent) << 1) ^
               (hash<glm::vec2>()(vertex.uv) << 1);
    }
};
}  // namespace std

VulkMeshRef VulkMesh::appendMesh(VulkMesh const& mesh) {
    uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
    vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    uint32_t indexOffset = static_cast<uint32_t>(indices.size());
    for (uint32_t index : mesh.indices) {
        indices.push_back(index + vertexOffset);
    }

    return VulkMeshRef{mesh.name, vertexOffset, indexOffset, static_cast<uint32_t>(mesh.indices.size())};
}

void VulkMesh::xform(glm::mat4 const& xform) {
    for (Vertex& v : vertices) {
        v.pos    = glm::vec3(xform * glm::vec4(v.pos, 1.0f));
        v.normal = glm::vec3(xform * glm::vec4(v.normal, 0.0f));
        // v.tangent = glm::vec3(xform * glm::vec4(v.tangent, 0.0f));
    }
}

VulkMesh VulkMesh::loadFromFile(char const* filename, std::string name) {
    VulkMesh model;
    loadModel(filename, model.vertices, model.indices);
    model.name = name;
    assert(model.vertices.size() > 0);
    assert(model.indices.size() > 0);
    return model;
}
