#pragma once
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
#include "VulkScene.h"
#include "VulkShaderModule.h"

using namespace std;
namespace fs = std::filesystem;

struct ShaderDef {
    string name;
    fs::path path;
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
};

struct DescriptorSetDef {
    unordered_map<VkShaderStageFlagBits, vector<VulkShaderUBOBindings>> uniformBuffers;
    unordered_map<VkShaderStageFlagBits, vector<VulkShaderSSBOBindings>> storageBuffers;
    unordered_map<VkShaderStageFlagBits, vector<VulkShaderTextureBindings>> imageSamplers;

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

    static vector<pair<string, VulkShaderUBOBindings>> const &getUBOBindings() {
        static vector<pair<string, VulkShaderUBOBindings>> bindings = {
            {"xforms", VulkShaderUBOBinding_Xforms},
            {"lights", VulkShaderUBOBinding_Lights},
            {"eyePos", VulkShaderUBOBinding_EyePos},
            {"modelXform", VulkShaderUBOBinding_ModelXform},
            {"materialUBO", VulkShaderUBOBinding_MaterialUBO},
            {"debugNormals", VulkShaderUBOBinding_DebugNormals},
            {"debugTangents", VulkShaderUBOBinding_DebugTangents},
        };
        return bindings;
        static_assert(VulkShaderUBOBinding_MaxBindingID == VulkShaderUBOBinding_DebugTangents, "this function must be kept in sync with VulkShaderUBOBindings");
    }

    static VulkShaderUBOBindings shaderUBOBindingFromStr(string const &binding) {
        static unordered_map<string, VulkShaderUBOBindings> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : getUBOBindings()) {
                assert(!bindings.contains(name));
                bindings[name] = value;
            }
        });
        return bindings.at(binding);
    }

    static vector<pair<string, VulkShaderTextureBindings>> const &getTextureBindings() {
        static vector<pair<string, VulkShaderTextureBindings>> bindings = {
            {"textureSampler", VulkShaderTextureBinding_TextureSampler},
            {"textureSampler2", VulkShaderTextureBinding_TextureSampler2},
            {"textureSampler3", VulkShaderTextureBinding_TextureSampler3},
            {"normalSampler", VulkShaderTextureBinding_NormalSampler},
        };
        return bindings;
    }

    static VulkShaderTextureBindings shaderTextureBindingFromStr(string const &binding) {
        static unordered_map<string, VulkShaderTextureBindings> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : getTextureBindings()) {
                assert(!bindings.contains(name));
                bindings[name] = value;
            }
        });
        return bindings.at(binding);
        static_assert(VulkShaderTextureBinding_MaxBindingID == VulkShaderTextureBinding_NormalSampler,
                      "this function must be kept in sync with VulkShaderTextureBindings");
    }

    static VulkShaderSSBOBindings shaderSSBOBindingFromStr(string const &binding) {
        static unordered_map<string, VulkShaderSSBOBindings> bindings{};
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

    static string shaderUBOBindingToStr(VulkShaderUBOBindings binding) {
        static unordered_map<VulkShaderUBOBindings, string> bindings;
        static once_flag flag;
        call_once(flag, [&]() {
            for (auto const &[name, value] : DescriptorSetDef::getUBOBindings()) {
                assert(!bindings.contains(value));
                bindings[value] = name;
            }
        });
        return bindings.at(binding);
    }

    static string shaderSSBOBindingToStr(VulkShaderSSBOBindings binding) {
        static unordered_map<VulkShaderSSBOBindings, string> bindings;
        return bindings.at(binding);
    }

    static string shaderTextureBindingToStr(VulkShaderTextureBindings binding) {
        static unordered_map<VulkShaderTextureBindings, string> bindings;
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

    static VkShaderStageFlagBits getShaderStageFromStr(std::string s) {
        static unordered_map<string, VkShaderStageFlagBits> shaderStageFromStr{
            {"vert", VK_SHADER_STAGE_VERTEX_BIT},
            {"frag", VK_SHADER_STAGE_FRAGMENT_BIT},
            {"geom", VK_SHADER_STAGE_GEOMETRY_BIT},
        };

        return shaderStageFromStr.at(s);
    }
};

#define PIPELINE_JSON_VERSION 1
// PipelineDeclDef is used to declare a pipeline that is transformed into a PipelineDef during the build process
// e.g. it lives in the Assets/Pipelines directory
struct PipelineDeclDef {
    string name;
    string vertShaderName;
    string geomShaderName;
    string fragShaderName;

    VkPrimitiveTopology primitiveTopology;
    bool depthTestEnabled;
    bool depthWriteEnabled;
    VkCompareOp depthCompareOp;

    uint32_t vertexInputBinding;
    DescriptorSetDef descriptorSet;

    void validate() {
        assert(!name.empty());
        assert(!vertShaderName.empty());
        assert(!fragShaderName.empty());
    }

    static VkPrimitiveTopology getPrimitiveTopologyFromStr(string s) {
        static unordered_map<string, VkPrimitiveTopology> primitiveTopologyMap{
            {"TriangleList", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST}, {"TriangleStrip", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
            {"LineList", VK_PRIMITIVE_TOPOLOGY_LINE_LIST},         {"LineStrip", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
            {"PointList", VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
        };

        return primitiveTopologyMap.at(s);
    }

    static VkCompareOp getDepthCompareOpFromStr(string s) {
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

        return depthCompareOpMap.at(s);
    }

    static PipelineDeclDef fromJSON(const nlohmann::json &j) {
        PipelineDeclDef p;
        assert(j.at("version").get<uint32_t>() == PIPELINE_JSON_VERSION);
        p.name = j.at("name").get<string>();
        p.vertShaderName = j.at("vertShader").get<string>();
        p.fragShaderName = j.at("fragShader").get<string>();
        p.geomShaderName = j.value("geomShader", "");
        p.primitiveTopology = PipelineDeclDef::getPrimitiveTopologyFromStr(j.value("primitiveTopology", "TriangleList"));
        p.depthTestEnabled = j.value("depthTestEnabled", true);
        p.depthWriteEnabled = j.value("depthWriteEnabled", true);
        p.depthCompareOp = getDepthCompareOpFromStr(j.value("depthCompareOp", "LESS"));
        p.vertexInputBinding = j.at("vertexInputBinding").get<uint32_t>();
        p.validate();
        return p;
    }
    static nlohmann::json toJSON(const PipelineDeclDef &def) {
        nlohmann::json j;
        j["version"] = PIPELINE_JSON_VERSION;
        j["name"] = def.name;
        j["vertShader"] = def.vertShaderName;
        j["fragShader"] = def.fragShaderName;
        j["primitiveTopology"] = "TriangleList"; // TODO: add support for other topologies
        j["depthTestEnabled"] = def.depthTestEnabled;
        j["depthWriteEnabled"] = def.depthWriteEnabled;
        j["depthCompareOp"] = "LESS"; // TODO: add support for other compare ops
        j["vertexInputBinding"] = def.vertexInputBinding;
        j["descriptorSet"] = DescriptorSetDef::toJSON(def.descriptorSet);
        if (!def.geomShaderName.empty())
            j["geomShader"] = def.geomShaderName;
        return j;
    }
};

struct PipelineDef : public PipelineDeclDef {
    shared_ptr<ShaderDef> vertShader;
    shared_ptr<ShaderDef> geomShader;
    shared_ptr<ShaderDef> fragShader;

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
        def.primitiveTopology = j.contains("primitiveTopology") ? PipelineDeclDef::getPrimitiveTopologyFromStr(j.at("primitiveTopology").get<string>())
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
        j["primitiveTopology"] = "TriangleList"; // TODO: add support for other topologies
        j["depthTestEnabled"] = def.depthTestEnabled;
        j["depthWriteEnabled"] = def.depthWriteEnabled;
        j["depthCompareOp"] = "LESS"; // TODO: add support for other compare ops
        j["vertexInputBinding"] = def.vertexInputBinding;
        j["descriptorSet"] = DescriptorSetDef::toJSON(def.descriptorSet);
        if (def.geomShader)
            j["geomShader"] = def.geomShader->name;
        return j;
    }
};

enum MeshDefType {
    MeshDefType_Model,
    MeshDefType_Mesh,
};

struct ModelMeshDef {
    fs::path path;
};

struct GeoMeshDef {
    VulkMesh mesh;
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
};

extern void findAndProcessMetadata(const fs::path &path, Metadata &metadata);
extern Metadata const *getMetadata();
