#pragma once
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
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

#include <cereal/cereal.hpp>
#include <glm/vec3.hpp>

template <typename T> struct EnumNameGetter;

template <> struct EnumNameGetter<VulkVertBindingLocation> {
    static const char *const *getNames() {
        return EnumNamesVulkVertBindingLocation();
    }
    static VulkVertBindingLocation getMin() {
        return VulkVertBindingLocation_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderBindings> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderBindings();
    }
    static VulkShaderBindings getMin() {
        return VulkShaderBindings_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderUBOBinding> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderUBOBinding();
    }
    static VulkShaderUBOBinding getMin() {
        return VulkShaderUBOBinding_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderDebugUBO> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderDebugUBO();
    }
    static VulkShaderDebugUBO getMin() {
        return VulkShaderDebugUBO_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderSSBOBinding> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderSSBOBinding();
    }
    static VulkShaderSSBOBinding getMin() {
        return VulkShaderSSBOBinding_MIN;
    }
};

template <> struct EnumNameGetter<VulkShaderTextureBinding> {
    static const char *const *getNames() {
        return EnumNamesVulkShaderTextureBinding();
    }
    static VulkShaderTextureBinding getMin() {
        return VulkShaderTextureBinding_MIN;
    }
};

template <> struct EnumNameGetter<VulkPrimitiveTopology> {
    static const char *const *getNames() {
        return EnumNamesVulkPrimitiveTopology();
    }
    static VulkPrimitiveTopology getMin() {
        return VulkPrimitiveTopology_MIN;
    }
};

template <> struct EnumNameGetter<MeshDefType> {
    static const char *const *getNames() {
        return EnumNamesMeshDefType();
    }
    static MeshDefType getMin() {
        return MeshDefType_MIN;
    }
};

template <> struct EnumNameGetter<GeoMeshDefType> {
    static const char *const *getNames() {
        return EnumNamesGeoMeshDefType();
    }
    static GeoMeshDefType getMin() {
        return GeoMeshDefType_MIN;
    }
};

template <> struct EnumNameGetter<VulkCompareOp> {
    static const char *const *getNames() {
        return EnumNamesVulkCompareOp();
    }
    static VulkCompareOp getMin() {
        return VulkCompareOp_MIN;
    }
};

// the strings are packed in the array starting from the min value
template <typename EnumType> struct EnumLookup {
    static EnumType getEnumFromStr(std::string value) {
        static std::unordered_map<std::string, EnumType> enumMap;
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            const char *const *vals = EnumNameGetter<EnumType>::getNames();
            EnumType min = EnumNameGetter<EnumType>::getMin();
            for (int i = 0; vals[i]; i++) {
                EnumType enumValue = static_cast<EnumType>(min + i);
                if (*vals[i])
                    enumMap[vals[i]] = enumValue;
            }
        });
        return enumMap.at(value);
    }
    static std::string getStrFromEnum(EnumType type) {
        return EnumNameGetter<EnumType>::getNames()[static_cast<int>(type) - EnumNameGetter<EnumType>::getMin()];
    }
};

#define FlatBufEnumSaveMinimal(EnumType)                                                                                                                       \
    template <class Archive> std::string save_minimal(Archive const &archive, EnumType const &type) {                                                          \
        return EnumLookup<EnumType>::getStrFromEnum(type);                                                                                                     \
    }                                                                                                                                                          \
    template <class Archive> void load_minimal(Archive const &archive, EnumType &type, std::string const &value) {                                             \
        type = EnumLookup<EnumType>::getEnumFromStr(value.c_str());                                                                                            \
    }

namespace cereal {
    FlatBufEnumSaveMinimal(MeshDefType);
    FlatBufEnumSaveMinimal(VulkShaderUBOBinding);
    FlatBufEnumSaveMinimal(VulkShaderSSBOBinding);
    FlatBufEnumSaveMinimal(VulkShaderTextureBinding);
    FlatBufEnumSaveMinimal(VulkPrimitiveTopology);
    FlatBufEnumSaveMinimal(VulkCompareOp);

    // this doesn't resolve properly :(
    // template <class Archive, typename EnumType> std::string save_minimal(Archive const &archive, EnumType const &type) {
    //     return EnumLookup<EnumType>::getStrFromEnum(type);
    // }
    // template <class Archive, typename EnumType> void load_minimal(Archive const &archive, EnumType &type, std::string const &value) {
    //     type = EnumLookup<EnumType>::getEnumFromStr(value.c_str());
    // }
} // namespace cereal

struct ShaderDef {
    string name;
    string path;

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(path));
    }
};

struct MaterialDef {
    string name;
    std::string mapKa;     // Ambient texture map
    std::string mapKd;     // Diffuse texture map
    std::string mapKs;     // Specular texture map
    std::string mapNormal; // Normal map (specified as 'bump' in the file)
    float Ns;              // Specular exponent (shininess)
    float Ni;              // Optical density (index of refraction)
    float d;               // Transparency (dissolve)
    glm::vec3 Ka;          // Ambient color
    glm::vec3 Kd;          // Diffuse color
    glm::vec3 Ks;          // Specular color
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

    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(uniformBuffers), CEREAL_NVP(storageBuffers), CEREAL_NVP(imageSamplers));
    }

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

    static void parseShaderStageMap(const nlohmann::json &j, VkShaderStageFlagBits stage, DescriptorSetDef &ds) {
        vector<string> uniformBuffers = j.at("uniformBuffers").get<vector<string>>();
        for (auto const &value : uniformBuffers) {
            ds.uniformBuffers[stage].push_back(EnumLookup<VulkShaderUBOBinding>::getEnumFromStr(value));
        }
        if (j.contains("storageBuffers")) {
            vector<string> storageBuffers = j.at("storageBuffers").get<vector<string>>();
            for (auto const &value : storageBuffers) {
                ds.storageBuffers[stage].push_back(EnumLookup<VulkShaderSSBOBinding>::getEnumFromStr(value));
            }
        }
        if (j.contains("imageSamplers")) {
            vector<string> imageSamplers = j.at("imageSamplers").get<vector<string>>();
            for (auto const &value : imageSamplers) {
                ds.imageSamplers[stage].push_back(EnumLookup<VulkShaderTextureBinding>::getEnumFromStr(value));
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

    static nlohmann::json toJSON(const DescriptorSetDef &def) {
        nlohmann::json j;
        for (auto const &[stage, bindings] : def.uniformBuffers) {
            std::vector<std::string> bindingStrs;
            for (auto const &binding : bindings) {
                bindingStrs.push_back(EnumLookup<VulkShaderUBOBinding>::getStrFromEnum(binding));
            }
            j[shaderStageToStr(stage)]["uniformBuffers"] = bindingStrs;
        }
        for (auto const &[stage, bindings] : def.storageBuffers) {
            std::vector<std::string> bindingStrs;
            for (auto const &binding : bindings) {
                bindingStrs.push_back(EnumLookup<VulkShaderSSBOBinding>::getStrFromEnum(binding));
            }
            j[shaderStageToStr(stage)]["storageBuffers"] = bindingStrs;
        }
        for (auto const &[stage, bindings] : def.imageSamplers) {
            std::vector<std::string> bindingStrs;
            for (auto const &binding : bindings) {
                bindingStrs.push_back(EnumLookup<VulkShaderTextureBinding>::getStrFromEnum(binding));
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
} // namespace cereal

struct PipelineDeclDef {
    string name;
    string vertShaderName;
    string geomShaderName; // optional
    string fragShaderName;

    VulkPrimitiveTopology primitiveTopology;
    bool depthTestEnabled;
    bool depthWriteEnabled;
    VulkCompareOp depthCompareOp;

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
        ar(CEREAL_NVP(name), cereal::make_nvp("vertShader", vertShaderName), cereal::make_nvp("geomShader", geomShaderName),
           cereal::make_nvp("fragShader", fragShaderName), CEREAL_NVP(primitiveTopology), CEREAL_NVP(depthTestEnabled), CEREAL_NVP(depthWriteEnabled),
           CEREAL_NVP(depthCompareOp), CEREAL_NVP(vertexInputBinding), CEREAL_NVP(descriptorSet), CEREAL_NVP(blending), CEREAL_NVP(cullMode));
    }

    void validate() {
        assert(!name.empty());
        assert(!vertShaderName.empty());
        assert(!fragShaderName.empty());
    }

    static PipelineDeclDef fromJSON(const nlohmann::json &j) {
        PipelineDeclDef p;
        p.name = j.at("name").get<string>();
        p.vertShaderName = j.at("vertShader").get<string>();
        p.fragShaderName = j.at("fragShader").get<string>();
        p.geomShaderName = j.value("geomShader", "");
        p.primitiveTopology = EnumLookup<VulkPrimitiveTopology>::getEnumFromStr(j.value("primitiveTopology", "TriangleList"));
        p.depthTestEnabled = j.value("depthTestEnabled", true);
        p.depthWriteEnabled = j.value("depthWriteEnabled", true);
        p.depthCompareOp = EnumLookup<VulkCompareOp>::getEnumFromStr(j.value("depthCompareOp", "LESS"));
        p.vertexInputBinding = j.at("vertexInputBinding").get<uint32_t>();
        if (j.contains("blending"))
            p.blending = Blending::fromJSON(j["blending"]);
        p.cullMode = j.value("cullMode", VK_CULL_MODE_BACK_BIT);
        p.validate();
        return p;
    }
    static nlohmann::json toJSON(const PipelineDeclDef &def) {
        nlohmann::json j;
        j["name"] = def.name;
        j["vertShader"] = def.vertShaderName;
        j["fragShader"] = def.fragShaderName;
        j["primitiveTopology"] = EnumLookup<VulkPrimitiveTopology>::getStrFromEnum(def.primitiveTopology);
        j["depthTestEnabled"] = def.depthTestEnabled;
        j["depthWriteEnabled"] = def.depthWriteEnabled;
        j["depthCompareOp"] = EnumLookup<VulkCompareOp>::getStrFromEnum(def.depthCompareOp);
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
        def.primitiveTopology = EnumLookup<VulkPrimitiveTopology>::getEnumFromStr(j.value("primitiveTopology", "TriangleList"));
        def.depthTestEnabled = j.value("depthTestEnabled", true);
        def.depthWriteEnabled = j.value("depthWriteEnabled", true);
        def.depthCompareOp = EnumLookup<VulkCompareOp>::getEnumFromStr(j.value("depthCompareOp", "LESS"));

        def.vertexInputBinding = j.at("vertexInputBinding").get<uint32_t>();
        if (j.contains("descriptorSet"))
            def.descriptorSet = DescriptorSetDef::fromJSON(j.at("descriptorSet")); // Use custom from_json for DescriptorSetDef

        def.validate();
        return def;
    }
    static nlohmann::json toJSON(const PipelineDef &def) {
        nlohmann::json j;
        j["name"] = def.name;
        j["vertShader"] = def.vertShader->name;
        j["fragShader"] = def.fragShader->name;
        j["primitiveTopology"] = EnumLookup<VulkPrimitiveTopology>::getStrFromEnum(def.primitiveTopology);
        j["depthTestEnabled"] = def.depthTestEnabled;
        j["depthWriteEnabled"] = def.depthWriteEnabled;
        j["depthCompareOp"] = EnumLookup<VulkCompareOp>::getStrFromEnum(def.depthCompareOp);
        j["vertexInputBinding"] = def.vertexInputBinding;
        j["descriptorSet"] = DescriptorSetDef::toJSON(def.descriptorSet);
        if (def.geomShader)
            j["geomShader"] = def.geomShader->name;
        return j;
    }
};

struct ModelMeshDef {
    std::string path;
    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(path));
    }
};

struct MeshDef {
    string name;
    MeshDefType type;
    MeshDef() = default;
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
    ModelDef() = default;
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