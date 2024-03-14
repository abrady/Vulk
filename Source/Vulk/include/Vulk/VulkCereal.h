#pragma once

#include "VulkUtil.h"

class VulkCereal {
  public:
    bool useJSON = false;
    static VulkCereal *inst() {
        static VulkCereal *instance = new VulkCereal();
        return instance;
    };

    template <typename T> void fromFile(std::filesystem::path path, T &obj) {
        std::ifstream file(path);
        if (!file.is_open()) {
            VULK_THROW("Failed to open file: " + path.string());
        }
        if (useJSON) {
            cereal::JSONInputArchive archive(file);
            obj.serialize(archive);
        } else {
            cereal::BinaryInputArchive archive(file);
            obj.serialize(archive);
        }
    }

    template <typename T> void toFile(std::filesystem::path path, T &obj) {
        std::ofstream file(path);
        if (!file.is_open()) {
            VULK_THROW("Failed to open file: " + path.string());
        }
        if (useJSON) {
            cereal::JSONOutputArchive archive(file);
            obj.serialize(archive);
        } else {
            cereal::BinaryOutputArchive archive(file);
            obj.serialize(archive);
        }
    }
};

namespace cereal {

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
} // namespace cereal

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
