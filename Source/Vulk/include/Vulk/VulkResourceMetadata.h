#pragma once
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include "VulkCamera.h"
#include "VulkCereal.h"
#include "VulkEnumMetadata.h"
#include "VulkGeo.h"
#include "VulkPointLight.h"
#include "VulkResourceMetadata_generated.h"
#include "VulkResourceMetadata_types.h"
#include "VulkScene.h"
#include "VulkShaderModule.h"

using namespace std;
namespace fs = std::filesystem;

#include <glm/vec3.hpp>

#include <cereal/cereal.hpp>

#define FlatBufEnumSaveMinimal(EnumType)                                        \
  template <class Archive>                                                      \
  std::string save_minimal(Archive const&, EnumType const& type) {              \
    return EnumLookup<EnumType>::getStrFromEnum(type);                          \
  }                                                                             \
  template <class Archive>                                                      \
  void load_minimal(Archive const&, EnumType& type, std::string const& value) { \
    type = EnumLookup<EnumType>::getEnumFromStr(value.c_str());                 \
  }

namespace cereal {
FlatBufEnumSaveMinimal(MeshDefType);
FlatBufEnumSaveMinimal(VulkShaderUBOBinding);
FlatBufEnumSaveMinimal(VulkShaderSSBOBinding);
FlatBufEnumSaveMinimal(VulkShaderTextureBinding);
FlatBufEnumSaveMinimal(VulkPrimitiveTopology);
FlatBufEnumSaveMinimal(VulkPolygonMode);
FlatBufEnumSaveMinimal(VulkCompareOp);
FlatBufEnumSaveMinimal(VulkCullModeFlags);

// this doesn't resolve properly :(
// template <class Archive, typename EnumType> std::string save_minimal(Archive const &archive, EnumType const &type) {
//     return EnumLookup<EnumType>::getStrFromEnum(type);
// }
// template <class Archive, typename EnumType> void load_minimal(Archive const &archive, EnumType &type, std::string
// const &value) {
//     type = EnumLookup<EnumType>::getEnumFromStr(value.c_str());
// }
} // namespace cereal

namespace cereal {
template <class Archive>
void serialize(Archive& ar, vulk::ShaderDef& d) {
  ar(CEREAL_NVP(d.name), CEREAL_NVP(d.path));
}
} // namespace cereal

struct MaterialDef {
  string name;
  std::string mapKa;     // Ambient texture map
  std::string mapKd;     // Diffuse texture map
  std::string mapKs;     // Specular texture map
  std::string mapNormal; // Normal map (specified as 'bump' in the file)
  std::string mapPm;     // Metallic map
  std::string mapPr;     // Roughness map
  std::string disp;      // Displacement map
  float Ns;              // Specular exponent (shininess)
  float Ni;              // Optical density (index of refraction)
  float d;               // Transparency (dissolve)
  glm::vec3 Ka;          // Ambient color
  glm::vec3 Kd;          // Diffuse color
  glm::vec3 Ks;          // Specular color
  // Initialize with default values
  MaterialDef()
    : Ns(0.0f)
    , Ni(1.0f)
    , d(1.0f)
    , Ka{0.0f, 0.0f, 0.0f}
    , Kd{0.0f, 0.0f, 0.0f}
    , Ks{0.0f, 0.0f, 0.0f} {}

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

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(name),
       CEREAL_NVP(mapKa),
       CEREAL_NVP(mapKd),
       CEREAL_NVP(mapKs),
       CEREAL_NVP(mapNormal),
       CEREAL_NVP(Ns),
       CEREAL_NVP(Ni),
       CEREAL_NVP(d),
       CEREAL_NVP(Ka),
       CEREAL_NVP(Kd),
       CEREAL_NVP(Ks));
  }
};

struct DescriptorSetDef {
  unordered_map<VkShaderStageFlagBits, vector<VulkShaderUBOBinding>> uniformBuffers;
  unordered_map<VkShaderStageFlagBits, vector<VulkShaderSSBOBinding>> storageBuffers;
  unordered_map<VkShaderStageFlagBits, vector<VulkShaderTextureBinding>> imageSamplers;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(uniformBuffers), CEREAL_NVP(storageBuffers), CEREAL_NVP(imageSamplers));
  }

  uint32_t hash() const {
    uint32_t h = 0;
    for (auto const& [stage, bindings] : uniformBuffers) {
      h ^= stage;
      for (auto const& binding : bindings) {
        h ^= binding;
      }
    }
    for (auto const& [stage, bindings] : storageBuffers) {
      h ^= stage;
      for (auto const& binding : bindings) {
        h ^= binding;
      }
    }
    for (auto const& [stage, bindings] : imageSamplers) {
      h ^= stage;
      for (auto const& binding : bindings) {
        h ^= binding;
      }
    }
    return h;
  }

  void validate() {}

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

  static void parseShaderStageMap(const nlohmann::json& j, VkShaderStageFlagBits stage, DescriptorSetDef& ds) {
    vector<string> uniformBuffers = j.at("uniformBuffers").get<vector<string>>();
    for (auto const& value : uniformBuffers) {
      ds.uniformBuffers[stage].push_back(EnumLookup<VulkShaderUBOBinding>::getEnumFromStr(value));
    }
    if (j.contains("storageBuffers")) {
      vector<string> storageBuffers = j.at("storageBuffers").get<vector<string>>();
      for (auto const& value : storageBuffers) {
        ds.storageBuffers[stage].push_back(EnumLookup<VulkShaderSSBOBinding>::getEnumFromStr(value));
      }
    }
    if (j.contains("imageSamplers")) {
      vector<string> imageSamplers = j.at("imageSamplers").get<vector<string>>();
      for (auto const& value : imageSamplers) {
        ds.imageSamplers[stage].push_back(EnumLookup<VulkShaderTextureBinding>::getEnumFromStr(value));
      }
    }
  }

  static DescriptorSetDef fromJSON(const nlohmann::json& j) {
    DescriptorSetDef ds;
    for (auto const& [stage, bindings] : j.items()) {
      VkShaderStageFlagBits vkStage = getShaderStageFromStr(stage);
      parseShaderStageMap(bindings, vkStage, ds);
    }
    return ds;
  }

  static nlohmann::json toJSON(const DescriptorSetDef& def) {
    nlohmann::json j;
    for (auto const& [stage, bindings] : def.uniformBuffers) {
      std::vector<std::string> bindingStrs;
      for (auto const& binding : bindings) {
        bindingStrs.push_back(EnumLookup<VulkShaderUBOBinding>::getStrFromEnum(binding));
      }
      j[shaderStageToStr(stage)]["uniformBuffers"] = bindingStrs;
    }
    for (auto const& [stage, bindings] : def.storageBuffers) {
      std::vector<std::string> bindingStrs;
      for (auto const& binding : bindings) {
        bindingStrs.push_back(EnumLookup<VulkShaderSSBOBinding>::getStrFromEnum(binding));
      }
      j[shaderStageToStr(stage)]["storageBuffers"] = bindingStrs;
    }
    for (auto const& [stage, bindings] : def.imageSamplers) {
      std::vector<std::string> bindingStrs;
      for (auto const& binding : bindings) {
        bindingStrs.push_back(EnumLookup<VulkShaderTextureBinding>::getStrFromEnum(binding));
      }
      j[shaderStageToStr(stage)]["imageSamplers"] = bindingStrs;
    }
    return j;
  }
};

namespace cereal {
template <class Archive>
std::string save_minimal(const Archive&, const VkShaderStageFlagBits& m) {
  return DescriptorSetDef::shaderStageToStr(m);
}

template <class Archive>
void load_minimal(const Archive&, VkShaderStageFlagBits& m, const std::string& value) {
  m = DescriptorSetDef::getShaderStageFromStr(value);
}
} // namespace cereal

// represents the original pipeline declaration in the assets dir
struct SourcePipelineDef {
  string name;
  string vertShaderName;
  string geomShaderName; // optional
  string fragShaderName;

  VkPrimitiveTopology primitiveTopology;
  VkPolygonMode polygonMode;
  bool depthTestEnabled;
  bool depthWriteEnabled;
  VkCompareOp depthCompareOp;
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

    static Blending fromJSON(const nlohmann::json& j) {
      Blending b;
      b.enabled = j.at("enabled").get<bool>();
      if (j.contains("colorMask"))
        b.colorMask = j.at("colorMask").get<string>();
      return b;
    }
    static nlohmann::json toJSON(const Blending& b) {
      nlohmann::json j;
      j["enabled"] = b.enabled;
      j["colorMask"] = b.colorMask;
      return j;
    }

    template <class Archive>
    void serialize(Archive& ar) {
      ar(CEREAL_NVP(enabled), CEREAL_NVP(colorMask));
    }
  } blending;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(name),
       cereal::make_nvp("vertShader", vertShaderName),
       cereal::make_nvp("geomShader", geomShaderName),
       cereal::make_nvp("fragShader", fragShaderName),
       CEREAL_NVP(primitiveTopology),
       CEREAL_NVP(polygonMode),
       CEREAL_NVP(depthTestEnabled),
       CEREAL_NVP(depthWriteEnabled),
       CEREAL_NVP(depthCompareOp),
       CEREAL_NVP(blending),
       CEREAL_NVP(cullMode));
  }

  void validate() {
    assert(!name.empty());
    assert(!vertShaderName.empty());
    assert(!fragShaderName.empty());
  }

  static SourcePipelineDef fromJSON(const nlohmann::json& j) {
    SourcePipelineDef p;
    p.name = j.at("name").get<string>();
    p.vertShaderName = j.at("vertShader").get<string>();
    p.fragShaderName = j.at("fragShader").get<string>();
    p.geomShaderName = j.value("geomShader", "");
    p.primitiveTopology = (VkPrimitiveTopology)EnumLookup<VulkPrimitiveTopology>::getEnumFromStr(
        j.value("primitiveTopology", "TriangleList"));
    p.polygonMode = (VkPolygonMode)EnumLookup<VulkPolygonMode>::getEnumFromStr(j.value("polygonMode", "FILL"));
    p.depthTestEnabled = j.value("depthTestEnabled", true);
    p.depthWriteEnabled = j.value("depthWriteEnabled", true);
    p.depthCompareOp = (VkCompareOp)EnumLookup<VulkCompareOp>::getEnumFromStr(j.value("depthCompareOp", "LESS"));
    if (j.contains("blending"))
      p.blending = Blending::fromJSON(j["blending"]);
    p.cullMode = (VkCullModeFlags)EnumLookup<VulkCullModeFlags>::getEnumFromStr(j.value("cullMode", "BACK"));
    p.validate();
    return p;
  }
};

struct BuiltPipelineDef : public SourcePipelineDef {
  shared_ptr<vulk::ShaderDef> vertShader;
  shared_ptr<vulk::ShaderDef> geomShader;
  shared_ptr<vulk::ShaderDef> fragShader;
  DescriptorSetDef descriptorSet;
  std::vector<VulkShaderLocation> vertInputs;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<SourcePipelineDef>(this),
       CEREAL_NVP(vertShader),
       CEREAL_NVP(geomShader),
       CEREAL_NVP(fragShader),
       CEREAL_NVP(descriptorSet),
       CEREAL_NVP(vertInputs));
  }

  void validate() {
    SourcePipelineDef::validate();
    assert(vertShader);
    assert(fragShader);
  }

  void fixup(
      unordered_map<string, shared_ptr<vulk::ShaderDef>> const& vertShaders,
      unordered_map<string, shared_ptr<vulk::ShaderDef>> const& geometryShaders,
      unordered_map<string, shared_ptr<vulk::ShaderDef>> const& fragmentShaders) {
    vertShader = vertShaders.at(vertShaderName);
    fragShader = fragmentShaders.at(fragShaderName);
    if (!geomShaderName.empty()) {
      geomShader = geometryShaders.at(geomShaderName);
    }
    validate();
  }

  static BuiltPipelineDef fromFile(
      const fs::path& path,
      unordered_map<string, shared_ptr<vulk::ShaderDef>> const& vertShaders,
      unordered_map<string, shared_ptr<vulk::ShaderDef>> const& geometryShaders,
      unordered_map<string, shared_ptr<vulk::ShaderDef>> const& fragmentShaders) {
    BuiltPipelineDef def;
    VulkCereal::inst()->fromFile(path, def);
    def.fixup(vertShaders, geometryShaders, fragmentShaders);
    return def;
  }
};

struct ModelMeshDef {
  std::string path;
  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(path));
  }
};

struct MeshDef {
  string name;
  MeshDefType type;
  MeshDef() = default;
  MeshDef(string name, ModelMeshDef model)
    : name(name)
    , type(MeshDefType_Model)
    , model(make_shared<ModelMeshDef>(model)){};
  MeshDef(string name, std::shared_ptr<VulkMesh> mesh)
    : name(name)
    , type(MeshDefType_Mesh)
    , mesh(mesh){};
  shared_ptr<ModelMeshDef> getModelMeshDef() {
    assert(type == MeshDefType_Model);
    return model;
  }
  shared_ptr<VulkMesh> getMesh() {
    assert(type == MeshDefType_Mesh);
    return mesh;
  }

  template <class Archive>
  void serialize(Archive& ar) {
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
  ModelDef(string name, shared_ptr<MeshDef> mesh, shared_ptr<MaterialDef> material)
    : name(name)
    , mesh(mesh)
    , material(material) {
    assert(!name.empty());
    assert(mesh);
  }

  static ModelDef fromJSON(
      const nlohmann::json& j,
      unordered_map<string, shared_ptr<MeshDef>> const& meshes,
      unordered_map<string, shared_ptr<MaterialDef>> materials);

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(name), CEREAL_NVP(mesh), CEREAL_NVP(material));
  }
};

struct ActorDef {
  string name;
  shared_ptr<BuiltPipelineDef> pipeline;
  shared_ptr<ModelDef> model;
  glm::mat4 xform = glm::mat4(1.0f);

  void validate() {
    assert(!name.empty());
    assert(pipeline);
    assert(model);
    assert(xform != glm::mat4(0.0f));
  }

  static ActorDef fromJSON(
      const nlohmann::json& j,
      unordered_map<string, shared_ptr<BuiltPipelineDef>> const& pipelines,
      unordered_map<string, shared_ptr<ModelDef>> const& models,
      unordered_map<string, shared_ptr<MeshDef>> meshes,
      unordered_map<string, shared_ptr<MaterialDef>> materials);

  template <class Archive>
  void serialize(Archive& ar) {
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

  static SceneDef fromJSON(
      const nlohmann::json& j,
      unordered_map<string, shared_ptr<BuiltPipelineDef>> const& pipelines,
      unordered_map<string, shared_ptr<ModelDef>> const& models,
      unordered_map<string, shared_ptr<MeshDef>> const& meshes,
      unordered_map<string, shared_ptr<MaterialDef>> const& materials);

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(name), CEREAL_NVP(camera), CEREAL_NVP(pointLights), CEREAL_NVP(actors), CEREAL_NVP(actorMap));
  }
};

// The metadata is valid up to the point of loading resources, but does
// not contain the resources themselves. The resources are loaded on demand.
struct Metadata {
  unordered_map<string, shared_ptr<MeshDef>> meshes;
  unordered_map<string, shared_ptr<vulk::ShaderDef>> vertShaders;
  unordered_map<string, shared_ptr<vulk::ShaderDef>> geometryShaders;
  unordered_map<string, shared_ptr<vulk::ShaderDef>> fragmentShaders;
  unordered_map<string, shared_ptr<MaterialDef>> materials;
  unordered_map<string, shared_ptr<ModelDef>> models;
  unordered_map<string, shared_ptr<BuiltPipelineDef>> pipelines;
  unordered_map<string, shared_ptr<SceneDef>> scenes;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(CEREAL_NVP(meshes),
       CEREAL_NVP(vertShaders),
       CEREAL_NVP(geometryShaders),
       CEREAL_NVP(fragmentShaders),
       CEREAL_NVP(materials),
       CEREAL_NVP(models),
       CEREAL_NVP(pipelines),
       CEREAL_NVP(scenes));
  }
};

extern void findAndProcessMetadata(const fs::path path, Metadata& metadata);
extern Metadata const* getMetadata();
extern std::filesystem::path getResourcesDir();