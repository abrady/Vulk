#pragma once

#include <filesystem>

#include "Vulk.h"

struct VulkMeshRef {
    std::string name;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;

    template <class Archive> void serialize(Archive &archive) {
        archive(CEREAL_NVP(pos), CEREAL_NVP(normal), CEREAL_NVP(tangent), CEREAL_NVP(uv));
    }
};

class VulkMesh {
  public:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VulkMeshRef appendMesh(VulkMesh const &mesh);
    void xform(glm::mat4 const &xform);

    static VulkMesh loadFromFile(char const *filename, std::string name);
    static VulkMesh loadFromPath(std::filesystem::path const &path, std::string name) {
        return loadFromFile(path.string().c_str(), name);
    }

    template <class Archive> void serialize(Archive &archive) {
        archive(name, vertices, indices);
    }
};
