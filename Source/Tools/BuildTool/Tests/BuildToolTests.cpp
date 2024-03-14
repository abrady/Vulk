#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

#include <filesystem>

#include "Vulk/VulkUtil.h"

#include "../PipelineBuilder.h"
// #include "VulkResourceMetadata_generated.h"

// this errors because this is just an int, not a real type
// template <class Archive> std::string save_minimal(const Archive &, const VkColorComponentFlags &m) {
//     string mask;
//     if (m & VK_COLOR_COMPONENT_R_BIT)
//         mask += "R";
//     if (m & VK_COLOR_COMPONENT_G_BIT)
//         mask += "G";
//     if (m & VK_COLOR_COMPONENT_B_BIT)
//         mask += "B";
//     if (m & VK_COLOR_COMPONENT_A_BIT)
//         mask += "A";
//     return mask;
// }

// template <class Archive> void load_minimal(const Archive &, VkColorComponentFlags &mask, const std::string &colorMask) {
//     VkColorComponentFlags mask = 0;
//     if (colorMask.find("R") != string::npos)
//         mask |= VK_COLOR_COMPONENT_R_BIT;
//     if (colorMask.find("G") != string::npos)
//         mask |= VK_COLOR_COMPONENT_G_BIT;
//     if (colorMask.find("B") != string::npos)
//         mask |= VK_COLOR_COMPONENT_B_BIT;
//     if (colorMask.find("A") != string::npos)
//         mask |= VK_COLOR_COMPONENT_A_BIT;
// }
//} // namespace cereal

namespace cereal {
    template <class Archive> void serialize(Archive &archive, glm::vec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::vec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::vec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::ivec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::ivec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::ivec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::uvec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::uvec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::uvec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::dvec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::dvec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::dvec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }

    // glm matrices serialization
    template <class Archive> void serialize(Archive &archive, glm::mat2 &m) {
        archive(m[0], m[1]);
    }
    template <class Archive> void serialize(Archive &archive, glm::dmat2 &m) {
        archive(m[0], m[1]);
    }
    template <class Archive> void serialize(Archive &archive, glm::mat3 &m) {
        archive(m[0], m[1], m[2]);
    }
    template <class Archive> void serialize(Archive &archive, glm::mat4 &m) {
        archive(m[0], m[1], m[2], m[3]);
    }
    template <class Archive> void serialize(Archive &archive, glm::dmat4 &m) {
        archive(m[0], m[1], m[2], m[3]);
    }

    template <class Archive> void serialize(Archive &archive, glm::quat &q) {
        archive(q.x, q.y, q.z, q.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::dquat &q) {
        archive(q.x, q.y, q.z, q.w);
    }

} // namespace cereal

namespace fs = std::filesystem;

struct MyData {
    int x;
    int y;
    glm::vec3 vec;
    // Serialize the MyData struct for JSON output
    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(vec));
    }
};

struct MaterialDef2 {
    std::string name;
    std::string mapKa; // Ambient texture map
    // std::string mapKd;     // Diffuse texture map
    // std::string mapKs;     // Specular texture map
    // std::string mapNormal; // Normal map (specified as 'bump' in the file)
    // float Ns;              // Specular exponent (shininess)
    // float Ni;              // Optical density (index of refraction)
    // float d;               // Transparency (dissolve)
    // glm::vec3 Ka;          // Ambient color
    // glm::vec3 Kd;          // Diffuse color
    // glm::vec3 Ks;          // Specular color

    template <class Archive> void serialize(Archive &ar) {
        ar(name, mapKa);
    }
};

template <class T> void testFile(std::string fn) {
    fs::path sceneFileIn = fs::path("Scenes") / fn;
    CHECK(fs::exists(sceneFileIn));
    std::ifstream sceneIn(sceneFileIn);
    cereal::JSONInputArchive input(sceneIn);
    T data;
    input(data); // Deserialize data from the file
    sceneIn.close();
    // Write to the output file
    std::ofstream sceneFileOut(fs::path("build") / fn);
    cereal::JSONOutputArchive output(sceneFileOut);
    output(data); // Serialize data to the file
    sceneFileOut.close();
}

TEST_CASE("SceneBuilder Tests") {
    // Read from the input file
    // SceneDef data;
    // ModelDef data("foo", nullptr, nullptr);
    // ModelMeshDef data2;
    // MeshDef data("foo", data2);
    // MaterialDef data;
    // MyData data;

    // testFile<MyData>("MyData.json");
}