#pragma once
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "VulkCamera.h"
#include "VulkGeo.h"
#include "VulkModel.h"
#include "VulkPointLight.h"
#include "VulkResourceMetadata_generated.h"
#include "VulkScene.h"
#include "VulkShaderModule.h"
using namespace std;
namespace fs = std::filesystem;

struct ShaderDef {
    string name;
    fs::path path;

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(path));
    }
};

struct MaterialDef {
    string name;
    fs::path mapKa;     // Ambient texture map
    fs::path mapKd;     // Diffuse texture map
    fs::path mapKs;     // Specular texture map
    fs::path mapNormal; // Normal map (specified as 'bump' in the file)
    float Ns;           // Specular exponent (shininess)
    float Ni;           // Optical density (index of refraction)
    float d;            // Transparency (dissolve)
    glm::vec3 Ka;       // Ambient color
    glm::vec3 Kd;       // Diffuse color
    glm::vec3 Ks;       // Specular color
    // Initialize with default values
    MaterialDef() : Ns(0.0f), Ni(1.0f), d(1.0f), Ka{0.0f, 0.0f, 0.0f}, Kd{0.0f, 0.0f, 0.0f}, Ks{0.0f, 0.0f, 0.0f} {
    }

    VulkMaterialConstants toVulkMaterialConstants() {
        VulkMaterialConstants m;
        m.Ka = Ka;
        m.Ns = Ns;
        m.Kd = Kd;
        m.Ni = Ni;
        m.Ks = Ks;
        m.d = d;
        return m;
    }

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(mapKa), CEREAL_NVP(mapKd), CEREAL_NVP(mapKs), CEREAL_NVP(mapNormal), CEREAL_NVP(Ns), CEREAL_NVP(Ni), CEREAL_NVP(d),
           CEREAL_NVP(Ka), CEREAL_NVP(Kd), CEREAL_NVP(Ks));
    }
};

struct DescriptorSetDef {
    unordered_map<VkShaderStageFlagBits, vector<VulkShaderUBOBinding>> uniformBuffers;
    unordered_map<VkShaderStageFlagBits, vector<VulkShaderSSBOBinding>> storageBuffers;
    unordered_map<VkShaderStageFlagBits, vector<VulkShaderTextureBinding>> imageSamplers;

    uint32_t hash() const {
        uint32_t h = 0;
        for (auto const &[stage, bindings] : uniformBuffers) {
            h ^= stage;
            for (auto const &binding : bindings) {
                h ^= binding;
            }
        }
        for (auto const &[stage, bindings] : storageBuffers) {
            h ^= stage;
            for (auto const &binding : bindings) {
                h ^= binding;
            }
        }
        for (auto const &[stage, bindings] : imageSamplers) {
            h ^= stage;
            for (auto const &binding : bindings) {
                h ^= binding;
            }
        }
        return h;
    }

    void validate() {
    }

    static string shaderStageToStr(VkShaderStageFlagBits stage) {
        static unordered_map<VkShaderStageFlagBits, string> shaderStageToStr{
            {VK_SHADER_STAGE_VERTEX_BIT, "vert"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "frag"},
            {VK_SHADER_STAGE_GEOMETRY_BIT, "geom"},
        };
        return shaderStageToStr.at(stage);
    }

    static VkShaderStageFlagBits getShaderStageFromStr(std::string s) {
        static unordered_map<string, VkShaderStageFlagBits> shaderStageFromStr{
            {"vert", VK_SHADER_STAGE_VERTEX_BIT},
            {"frag", VK_SHADER_STAGE_FRAGMENT_BIT},
            {"geom", VK_SHADER_STAGE_GEOMETRY_BIT},
        };

        return shaderStageFromStr.at(s);
    }

    static vector<pair<string, VulkShaderUBOBinding>> const &getUBOBindings() {
        static vector<pair<string, VulkShaderUBOBinding>> bindings = {
            {"xforms", VulkShaderUBOBinding_Xforms},
            {"lights", VulkShaderUBOBinding_Lights},
            {"eyePos", VulkShaderUBOBinding_EyePos},
            {"modelXform", VulkShaderUBOBinding_ModelXform},
            {"materialUBO", VulkShaderUBOBinding_MaterialUBO},
            {"debugNormals", VulkShaderUBOBinding_DebugNormals},
            {"debugTangents", VulkShaderUBOBinding_DebugTangents},
            {"lightViewProj", VulkShaderUBOBinding_LightViewProjUBO},
        };
        return bindings;
        static_assert(VulkShaderUBOBinding_MAX == VulkShaderUBOBinding_LightViewProjUBO, "this function must be kept in sync with VulkShaderUBOBinding");
    }

    static VulkShaderUBOBinding shaderUBOBindingFromStr(string const &binding) {
        static unordered_map<string, VulkShaderUBOBinding> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : getUBOBindings()) {
                assert(!bindings.contains(name));
                bindings[name] = value;
            }
        });
        return bindings.at(binding);
    }

    static vector<pair<string, VulkShaderTextureBinding>> const &getTextureBindings() {
        static vector<pair<string, VulkShaderTextureBinding>> bindings = {
            {"textureSampler", VulkShaderTextureBinding_TextureSampler},   {"textureSampler2", VulkShaderTextureBinding_TextureSampler2},
            {"textureSampler3", VulkShaderTextureBinding_TextureSampler3}, {"normalSampler", VulkShaderTextureBinding_NormalSampler},
            {"shadowSampler", VulkShaderTextureBinding_ShadowMapSampler},
        };
        static_assert(VulkShaderTextureBinding_MAX == VulkShaderTextureBinding_ShadowMapSampler,
                      "this function must be kept in sync with VulkShaderTextureBinding");
        return bindings;
    }

    static VulkShaderTextureBinding shaderTextureBindingFromStr(string const &binding) {
        static unordered_map<string, VulkShaderTextureBinding> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : getTextureBindings()) {
                assert(!bindings.contains(name));
                bindings[name] = value;
            }
        });
        return bindings.at(binding);
    }

    static VulkShaderSSBOBinding shaderSSBOBindingFromStr(string const &binding) {
        static unordered_map<string, VulkShaderSSBOBinding> bindings{};
        return bindings.at(binding);
    }

    static void parseShaderStageMap(const nlohmann::json &j, VkShaderStageFlagBits stage, DescriptorSetDef &ds) {
        vector<string> uniformBuffers = j.at("uniformBuffers").get<vector<string>>();
        for (auto const &value : uniformBuffers) {
            ds.uniformBuffers[stage].push_back(shaderUBOBindingFromStr(value));
        }
        if (j.contains("storageBuffers")) {
            vector<string> storageBuffers = j.at("storageBuffers").get<vector<string>>();
            for (auto const &value : storageBuffers) {
                ds.storageBuffers[stage].push_back(shaderSSBOBindingFromStr(value));
            }
        }
        if (j.contains("imageSamplers")) {
            vector<string> imageSamplers = j.at("imageSamplers").get<vector<string>>();
            for (auto const &value : imageSamplers) {
                ds.imageSamplers[stage].push_back(shaderTextureBindingFromStr(value));
            }
        }
    }

    static DescriptorSetDef fromJSON(const nlohmann::json &j) {
        DescriptorSetDef ds;
        for (auto const &[stage, bindings] : j.items()) {
            VkShaderStageFlagBits vkStage = getShaderStageFromStr(stage);
            parseShaderStageMap(bindings, vkStage, ds);
        }
        return ds;
    }

    static string shaderUBOBindingToStr(VulkShaderUBOBinding binding) {
        static unordered_map<VulkShaderUBOBinding, string> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : DescriptorSetDef::getUBOBindings()) {
                assert(!bindings.contains(value));
                bindings[value] = name;
            }
        });
        return bindings.at(binding);
    }

    static string shaderSSBOBindingToStr(VulkShaderSSBOBinding binding) {
        static unordered_map<VulkShaderSSBOBinding, string> bindings;
        return bindings.at(binding);
    }

    static string shaderTextureBindingToStr(VulkShaderTextureBinding binding) {
        static unordered_map<VulkShaderTextureBinding, string> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : DescriptorSetDef::getTextureBindings()) {
                assert(!bindings.contains(value));
                bindings[value] = name;
            }
        });
        return bindings.at(binding);
    }

    static nlohmann::json toJSON(const DescriptorSetDef &def) {
        nlohmann::json j;
        for (auto const &[stage, bindings] : def.uniformBuffers) {
            std::vector<std::string> bindingStrs;
            for (auto const &binding : bindings) {
                bindingStrs.push_back(shaderUBOBindingToStr(binding));
            }
            j[shaderStageToStr(stage)]["uniformBuffers"] = bindingStrs;
        }
        for (auto const &[stage, bindings] : def.storageBuffers) {
            std::vector<std::string> bindingStrs;
            for (auto const &binding : bindings) {
                bindingStrs.push_back(shaderSSBOBindingToStr(binding));
            }
            j[shaderStageToStr(stage)]["storageBuffers"] = bindingStrs;
        }
        for (auto const &[stage, bindings] : def.imageSamplers) {
            std::vector<std::string> bindingStrs;
            for (auto const &binding : bindings) {
                bindingStrs.push_back(shaderTextureBindingToStr(binding));
            }
            j[shaderStageToStr(stage)]["imageSamplers"] = bindingStrs;
        }
        return j;
    }
};

namespace cereal {
    template <class Archive> std::string save_minimal(const Archive &, const VkShaderStageFlagBits &m) {
        return DescriptorSetDef::shaderStageToStr(m);
    }

    template <class Archive> void load_minimal(const Archive &, VkShaderStageFlagBits &m, const std::string &value) {
        m = DescriptorSetDef::getShaderStageFromStr(value);
    }

    template <class Archive> std::string save_minimal(const Archive &, const VulkShaderUBOBinding &m) {
        return EnumNameVulkShaderUBOBinding(m);
    }

    template <class Archive> void load_minimal(const Archive &, VulkShaderUBOBinding &m, const std::string &value) {
        static unordered_map<string, VulkShaderUBOBinding> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            const char *const *names = EnumNamesVulkShaderUBOBinding();
            for (int i = 0; names[i]; i++) {
                auto name = names[i];
                if (*name)
                    bindings[name] = i;
            }
        });
        m = bindings.at(value);
    }

    template <class Archive> std::string save_minimal(const Archive &, const VulkShaderSSBOBinding &m) {
        return EnumNameVulkShaderSSBOBinding(m);
    }

    template <class Archive> void load_minimal(const Archive &, VulkShaderSSBOBinding &m, const std::string &value) {
        static unordered_map<string, VulkShaderSSBOBinding> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            const char *const *names = EnumNamesVulkShaderSSBOBinding();
            for (int i = 0; names[i]; i++) {
                auto name = names[i];
                if (*name)
                    bindings[name] = i;
            }
        });
        m = bindings.at(value);
    }

    template <class Archive> std::string save_minimal(const Archive &, const VulkShaderTextureBinding &m) {
        return EnumNameVulkShaderTextureBinding(m);
    }

    template <class Archive> void load_minimal(const Archive &, VulkShaderTextureBinding &m, const std::string &value) {
        static unordered_map<string, VulkShaderTextureBinding> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            const char *const *names = EnumNamesVulkShaderTextureBinding();
            for (int i = 0; names[i]; i++) {
                auto name = names[i];
                if (*name)
                    bindings[name] = i;
            }
        });
        m = bindings.at(value);
    }
} // namespace cereal

#define PIPELINE_JSON_VERSION 1

namespace cereal {
    template <class Archive> std::string save_minimal(const Archive &, const VkColorComponentFlags &m) {
        string mask;
        if (m & VK_COLOR_COMPONENT_R_BIT)
            mask += "R";
        if (m & VK_COLOR_COMPONENT_G_BIT)
            mask += "G";
        if (m & VK_COLOR_COMPONENT_B_BIT)
            mask += "B";
        if (m & VK_COLOR_COMPONENT_A_BIT)
            mask += "A";
        return mask;
    }

    template <class Archive> void load_minimal(const Archive &, VkColorComponentFlags &mask, const std::string &colorMask) {
        VkColorComponentFlags mask = 0;
        if (colorMask.find("R") != string::npos)
            mask |= VK_COLOR_COMPONENT_R_BIT;
        if (colorMask.find("G") != string::npos)
            mask |= VK_COLOR_COMPONENT_G_BIT;
        if (colorMask.find("B") != string::npos)
            mask |= VK_COLOR_COMPONENT_B_BIT;
        if (colorMask.find("A") != string::npos)
            mask |= VK_COLOR_COMPONENT_A_BIT;
    }
} // namespace cereal

struct PipelineDeclDef {
    string name;
    string vertShaderName;
    string geomShaderName; // optional
    string fragShaderName;

    VkPrimitiveTopology primitiveTopology;
    bool depthTestEnabled;
    bool depthWriteEnabled;
    VkCompareOp depthCompareOp;

    uint32_t vertexInputBinding;
    DescriptorSetDef descriptorSet;

    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;

    struct Blending {
        bool enabled = false;
        string colorMask = "RGBA";
        VkColorComponentFlags getColorMask() {
            VkColorComponentFlags mask = 0;
            if (colorMask.find("R") != string::npos)
                mask |= VK_COLOR_COMPONENT_R_BIT;
            if (colorMask.find("G") != string::npos)
                mask |= VK_COLOR_COMPONENT_G_BIT;
            if (colorMask.find("B") != string::npos)
                mask |= VK_COLOR_COMPONENT_B_BIT;
            if (colorMask.find("A") != string::npos)
                mask |= VK_COLOR_COMPONENT_A_BIT;
            return mask;
        }

        static Blending fromJSON(const nlohmann::json &j) {
            Blending b;
            b.enabled = j.at("enabled").get<bool>();
            if (j.contains("colorMask"))
                b.colorMask = j.at("colorMask").get<string>();
            return b;
        }
        static nlohmann::json toJSON(const Blending &b) {
            nlohmann::json j;
            j["enabled"] = b.enabled;
            j["colorMask"] = b.colorMask;
            return j;
        }

        template <class Archive> void serialize(Archive &ar) {
            ar(CEREAL_NVP(enabled), CEREAL_NVP(colorMask));
        }
    } blending;

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(vertShaderName), CEREAL_NVP(geomShaderName), CEREAL_NVP(fragShaderName), CEREAL_NVP(primitiveTopology),
           CEREAL_NVP(depthTestEnabled), CEREAL_NVP(depthWriteEnabled), CEREAL_NVP(depthCompareOp), CEREAL_NVP(vertexInputBinding), CEREAL_NVP(descriptorSet),
           CEREAL_NVP(blending), CEREAL_NVP(cullMode));
    }

    void validate() {
        assert(!name.empty());
        assert(!vertShaderName.empty());
        assert(!fragShaderName.empty());
    }

    static std::unordered_map<std::string, VkPrimitiveTopology> getPrimitiveTopologyMap() {
        static std::unordered_map<std::string, VkPrimitiveTopology> primitiveTopologyMap{
            {"TriangleList", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
            {"TriangleStrip", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
            {"LineList", VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
            {"LineStrip", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
            {"PointList", VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
            {"TriangleFan", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN},
            {"LineListWithAdjacency", VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY},
            {"LineStripWithAdjacency", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY},
            {"TriangleListWithAdjacency", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY},
            {"TriangleStripWithAdjacency", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY},
            {"PatchList", VK_PRIMITIVE_TOPOLOGY_PATCH_LIST},
        };
        return primitiveTopologyMap;
    };

    static VkPrimitiveTopology primitiveTopologyFromStr(string s) {
        return getPrimitiveTopologyMap().at(s);
    }

    static std::string primitiveTopologyToStr(VkPrimitiveTopology topology) {
        static std::unordered_map<VkPrimitiveTopology, std::string> primitiveTopologyToStr;
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            for (auto const &[name, value] : getPrimitiveTopologyMap()) {
                primitiveTopologyToStr[value] = name;
            }
        });
        return primitiveTopologyToStr.at(topology);
    }

    static unordered_map<string, VkCompareOp> getDepthCompareOpMap() {
        static unordered_map<string, VkCompareOp> depthCompareOpMap{
            {"NEVER", VK_COMPARE_OP_NEVER},
            {"LESS", VK_COMPARE_OP_LESS},
            {"EQUAL", VK_COMPARE_OP_EQUAL},
            {"LESS_OR_EQUAL", VK_COMPARE_OP_LESS_OR_EQUAL},
            {"GREATER", VK_COMPARE_OP_GREATER},
            {"NOT_EQUAL", VK_COMPARE_OP_NOT_EQUAL},
            {"GREATER_OR_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL},
            {"ALWAYS", VK_COMPARE_OP_ALWAYS},
        };
        return depthCompareOpMap;
    }
    static std::string depthCompareOpToStr(VkCompareOp op) {
        static std::unordered_map<VkCompareOp, std::string> depthCompareOpToStr;
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            for (auto const &[name, value] : getDepthCompareOpMap()) {
                depthCompareOpToStr[value] = name;
            }
        });
        return depthCompareOpToStr.at(op);
    }

    static VkCompareOp getDepthCompareOpFromStr(string s) {
        return getDepthCompareOpMap().at(s);
    }

    static PipelineDeclDef fromJSON(const nlohmann::json &j) {
        PipelineDeclDef p;
        assert(j.at("version").get<uint32_t>() == PIPELINE_JSON_VERSION);
        p.name = j.at("name").get<string>();
        p.vertShaderName = j.at("vertShader").get<string>();
        p.fragShaderName = j.at("fragShader").get<string>();
        p.geomShaderName = j.value("geomShader", "");
        p.primitiveTopology = PipelineDeclDef::primitiveTopologyFromStr(j.value("primitiveTopology", "TriangleList"));
        p.depthTestEnabled = j.value("depthTestEnabled", true);
        p.depthWriteEnabled = j.value("depthWriteEnabled", true);
        p.depthCompareOp = getDepthCompareOpFromStr(j.value("depthCompareOp", "LESS"));
        p.vertexInputBinding = j.at("vertexInputBinding").get<uint32_t>();
        if (j.contains("blending"))
            p.blending = Blending::fromJSON(j["blending"]);
        p.cullMode = j.value("cullMode", VK_CULL_MODE_BACK_BIT);
        p.validate();
        return p;
    }
    static nlohmann::json toJSON(const PipelineDeclDef &def) {
        nlohmann::json j;
        j["version"] = PIPELINE_JSON_VERSION;
        j["name"] = def.name;
        j["vertShader"] = def.vertShaderName;
        j["fragShader"] = def.fragShaderName;
        j["primitiveTopology"] = PipelineDeclDef::primitiveTopologyToStr(def.primitiveTopology);
        j["depthTestEnabled"] = def.depthTestEnabled;
        j["depthWriteEnabled"] = def.depthWriteEnabled;
        j["depthCompareOp"] = PipelineDeclDef::depthCompareOpToStr(def.depthCompareOp);
        j["vertexInputBinding"] = def.vertexInputBinding;
        j["descriptorSet"] = DescriptorSetDef::toJSON(def.descriptorSet);
        if (!def.geomShaderName.empty())
            j["geomShader"] = def.geomShaderName;
        j["blending"] = Blending::toJSON(def.blending);
        return j;
    }
};

struct PipelineDef : public PipelineDeclDef {
    shared_ptr<ShaderDef> vertShader;
    shared_ptr<ShaderDef> geomShader;
    shared_ptr<ShaderDef> fragShader;

    template <class Archive> void serialize(Archive &ar) {
        ar(cereal::base_class<PipelineDeclDef>(this), CEREAL_NVP(vertShader), CEREAL_NVP(geomShader), CEREAL_NVP(fragShader));
    }

    void validate() {
        PipelineDeclDef::validate();
        assert(vertShader);
        assert(fragShader);
    }

    static PipelineDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<ShaderDef>> const &vertShaders,
                                unordered_map<string, shared_ptr<ShaderDef>> const &geometryShaders,
                                unordered_map<string, shared_ptr<ShaderDef>> const &fragmentShaders) {
        PipelineDef def = {};

        PipelineDeclDef *p = &def; // deliberately slicing this here
        *p = PipelineDeclDef::fromJSON(j);

        def.vertShader = vertShaders.at(j.at("vertShader").get<string>());
        def.fragShader = fragmentShaders.at(j.at("fragShader").get<string>());
        if (j.contains("geomShader")) {
            def.geomShader = geometryShaders.at(j.at("geomShader").get<string>());
        }
        def.primitiveTopology = j.contains("primitiveTopology") ? PipelineDeclDef::primitiveTopologyFromStr(j.at("primitiveTopology").get<string>())
                                                                : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        def.depthTestEnabled = j.value("depthTestEnabled", true);
        def.depthWriteEnabled = j.value("depthWriteEnabled", true);
        def.depthCompareOp = PipelineDeclDef::getDepthCompareOpFromStr(j.value("depthCompareOp", "LESS"));

        def.vertexInputBinding = j.at("vertexInputBinding").get<uint32_t>();
        if (j.contains("descriptorSet"))
            def.descriptorSet = DescriptorSetDef::fromJSON(j.at("descriptorSet")); // Use custom from_json for DescriptorSetDef

        def.validate();
        return def;
    }
    static nlohmann::json toJSON(const PipelineDef &def) {
        nlohmann::json j;
        j["version"] = PIPELINE_JSON_VERSION;
        j["name"] = def.name;
        j["vertShader"] = def.vertShader->name;
        j["fragShader"] = def.fragShader->name;
        j["primitiveTopology"] = PipelineDeclDef::primitiveTopologyToStr(def.primitiveTopology);
        j["depthTestEnabled"] = def.depthTestEnabled;
        j["depthWriteEnabled"] = def.depthWriteEnabled;
        j["depthCompareOp"] = PipelineDeclDef::depthCompareOpToStr(def.depthCompareOp);
        j["vertexInputBinding"] = def.vertexInputBinding;
        j["descriptorSet"] = DescriptorSetDef::toJSON(def.descriptorSet);
        if (def.geomShader)
            j["geomShader"] = def.geomShader->name;
        return j;
    }
};

struct ModelMeshDef {
    fs::path path;
    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(path));
    }
};

struct MeshDef {
    string name;
    MeshDefType type;
    MeshDef(string name, ModelMeshDef model) : name(name), type(MeshDefType_Model), model(make_shared<ModelMeshDef>(model)){};
    MeshDef(string name, std::shared_ptr<VulkMesh> mesh) : name(name), type(MeshDefType_Mesh), mesh(mesh){};
    shared_ptr<ModelMeshDef> getModel() {
        assert(type == MeshDefType_Model);
        return model;
    }
    shared_ptr<VulkMesh> getMesh() {
        assert(type == MeshDefType_Mesh);
        return mesh;
    }

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(type));
        if (type == MeshDefType_Model) {
            ar(CEREAL_NVP(model));
        } else {
            ar(CEREAL_NVP(mesh));
        }
    }

  private:
    shared_ptr<ModelMeshDef> model;
    shared_ptr<VulkMesh> mesh;
};

#define MODEL_JSON_VERSION 1
struct ModelDef {
    string name;
    shared_ptr<MeshDef> mesh;
    shared_ptr<MaterialDef> material;

    ModelDef(string name, shared_ptr<MeshDef> mesh, shared_ptr<MaterialDef> material) : name(name), mesh(mesh), material(material) {
        assert(!name.empty());
        assert(mesh);
    }

    static ModelDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<MeshDef>> const &meshes,
                             unordered_map<string, shared_ptr<MaterialDef>> materials);

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(mesh), CEREAL_NVP(material));
    }
};

struct ActorDef {
    string name;
    shared_ptr<PipelineDef> pipeline;
    shared_ptr<ModelDef> model;
    glm::mat4 xform = glm::mat4(1.0f);

    void validate() {
        assert(!name.empty());
        assert(pipeline);
        assert(model);
        assert(xform != glm::mat4(0.0f));
    }

    static ActorDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<PipelineDef>> const &pipelines,
                             unordered_map<string, shared_ptr<ModelDef>> const &models, unordered_map<string, shared_ptr<MeshDef>> meshes,
                             unordered_map<string, shared_ptr<MaterialDef>> materials);

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(pipeline), CEREAL_NVP(model), CEREAL_NVP(xform));
    }
};

#define SCENE_JSON_VERSION 1
struct SceneDef {
    string name;
    VulkCamera camera;
    vector<shared_ptr<VulkPointLight>> pointLights;
    vector<shared_ptr<ActorDef>> actors;
    unordered_map<string, shared_ptr<ActorDef>> actorMap;

    void validate() {
        assert(!name.empty());
        assert(!actors.empty());
    }

    static SceneDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<PipelineDef>> const &pipelines,
                             unordered_map<string, shared_ptr<ModelDef>> const &models, unordered_map<string, shared_ptr<MeshDef>> const &meshes,
                             unordered_map<string, shared_ptr<MaterialDef>> const &materials);

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(camera), CEREAL_NVP(pointLights), CEREAL_NVP(actors), CEREAL_NVP(actorMap));
    }
};

// The metadata is valid up to the point of loading resources, but does
// not contain the resources themselves. The resources are loaded on demand.
struct Metadata {
    unordered_map<string, shared_ptr<MeshDef>> meshes;
    unordered_map<string, shared_ptr<ShaderDef>> vertShaders;
    unordered_map<string, shared_ptr<ShaderDef>> geometryShaders;
    unordered_map<string, shared_ptr<ShaderDef>> fragmentShaders;
    unordered_map<string, shared_ptr<MaterialDef>> materials;
    unordered_map<string, shared_ptr<ModelDef>> models;
    unordered_map<string, shared_ptr<PipelineDef>> pipelines;
    unordered_map<string, shared_ptr<SceneDef>> scenes;

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(meshes), CEREAL_NVP(vertShaders), CEREAL_NVP(geometryShaders), CEREAL_NVP(fragmentShaders), CEREAL_NVP(materials), CEREAL_NVP(models),
           CEREAL_NVP(pipelines), CEREAL_NVP(scenes));
    }
};

extern void findAndProcessMetadata(const fs::path path, Metadata &metadata);
extern Metadata const *getMetadata();
extern std::filesystem::path getResourcesDir();