#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#pragma warning(push)
#pragma warning(disable : 4702)  // unreachable code
#include <thrift/lib/cpp2/protocol/Serializer.h>
#pragma warning(pop)

#include "VulkCamera.h"
#include "VulkGeo.h"
#include "VulkPointLight.h"
#include "VulkScene.h"
#include "VulkShaderModule.h"

using namespace std;
namespace fs = std::filesystem;

#include <glm/vec3.hpp>

template <typename T>
void readDefFromFile(const std::string& path, T& def) {
    std::ifstream ifs(path);
    VULK_ASSERT(ifs.is_open(), "Could not open file for reading: %s", path.c_str());

    std::string serializedData((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    // Deserialize JSON to Thrift object
    apache::thrift::SimpleJSONSerializer::deserialize(serializedData, def);
}

template <typename T>
void writeDefToFile(const std::string& path, const T& def) {
    std::string serializedData;
    apache::thrift::SimpleJSONSerializer::serialize(def, &serializedData);

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }
    ofs << serializedData;
    ofs.close();
}

inline VkColorComponentFlags getColorMask(std::string colorMask) {
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

struct MaterialDef {
    string name;
    std::string mapKa;                       // Ambient texture map
    std::string mapKd;                       // Diffuse texture map
    std::string mapKs;                       // Specular texture map
    std::string mapNormal;                   // Normal map (specified as 'bump' in the file)
    std::string mapPm;                       // Metallic map
    std::string mapPr;                       // Roughness map
    std::string disp;                        // Displacement map
    std::array<std::string, 6> cubemapImgs;  // Cubemap: pos-x, neg-x, pos-y, neg-y, pos-z, neg-z
    float Ns;                                // Specular exponent (shininess)
    float Ni;                                // Optical density (index of refraction)
    float d;                                 // Transparency (dissolve)
    glm::vec3 Ka;                            // Ambient color
    glm::vec3 Kd;                            // Diffuse color
    glm::vec3 Ks;                            // Specular color
    // Initialize with default values
    MaterialDef() : Ns(0.0f), Ni(1.0f), d(1.0f), Ka{0.0f, 0.0f, 0.0f}, Kd{0.0f, 0.0f, 0.0f}, Ks{0.0f, 0.0f, 0.0f} {}

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

struct PipelineDef {
    vulk::cpp2::PipelineDef def;
    shared_ptr<vulk::cpp2::ShaderDef> vertShader;
    shared_ptr<vulk::cpp2::ShaderDef> geomShader;
    shared_ptr<vulk::cpp2::ShaderDef> fragShader;

    void validate() {
        assert(vertShader);
        assert(fragShader);
    }

    void fixup(unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> const& vertShaders,
               unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> const& geometryShaders,
               unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> const& fragmentShaders) {
        vertShader = vertShaders.at(def.vertShader().value());
        fragShader = fragmentShaders.at(def.fragShader().value());
        if (!def.geomShader()->empty()) {
            geomShader = geometryShaders.at(def.geomShader().value());
        }
        validate();
    }

    static PipelineDef fromFile(const fs::path& path,
                                unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> const& vertShaders,
                                unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> const& geometryShaders,
                                unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> const& fragmentShaders) {
        PipelineDef pipe;

        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;
        // Read from file
        readDefFromFile(path.string(), pipe.def);

        pipe.fixup(vertShaders, geometryShaders, fragmentShaders);
        return pipe;
    }
};

struct ModelMeshDef {
    std::string path;
};

struct MeshDef {
    string name;
    vulk::cpp2::MeshDefType type;
    MeshDef() = default;
    MeshDef(string name, ModelMeshDef model)
        : name(name), type(vulk::cpp2::MeshDefType::Model), model(make_shared<ModelMeshDef>(model)) {};
    MeshDef(string name, std::shared_ptr<VulkMesh> mesh)
        : name(name), type(vulk::cpp2::MeshDefType::Mesh), mesh(mesh) {};
    shared_ptr<ModelMeshDef> getModelMeshDef() {
        assert(type == vulk::cpp2::MeshDefType::Model);
        return model;
    }
    shared_ptr<VulkMesh> getMesh() {
        assert(type == vulk::cpp2::MeshDefType::Mesh);
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
    ModelDef() = default;
    ModelDef(string name, shared_ptr<MeshDef> mesh, shared_ptr<MaterialDef> material)
        : name(name), mesh(mesh), material(material) {
        assert(!name.empty());
        assert(mesh);
    }

    static ModelDef fromDef(vulk::cpp2::ModelDef const& def,
                            unordered_map<string, shared_ptr<MeshDef>> const& meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> materials);
};

struct ActorDef {
    vulk::cpp2::ActorDef def;
    shared_ptr<PipelineDef> pipeline;
    shared_ptr<ModelDef> model;
    glm::mat4 xform = glm::mat4(1.0f);

    void validate() {
        assert(!def.get_name().empty());
        assert(pipeline);
        assert(model);
        assert(xform != glm::mat4(0.0f));
    }

    static ActorDef fromDef(vulk::cpp2::ActorDef defIn,
                            unordered_map<string, shared_ptr<PipelineDef>> const& pipelines,
                            unordered_map<string, shared_ptr<ModelDef>> const& models,
                            unordered_map<string, shared_ptr<MeshDef>> meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> materials);
};

#define SCENE_JSON_VERSION 1
struct SceneDef {
    vulk::cpp2::SceneDef def;
    VulkCamera camera;
    vector<shared_ptr<VulkPointLight>> pointLights;
    vector<shared_ptr<ActorDef>> actors;
    unordered_map<string, shared_ptr<ActorDef>> actorMap;

    void validate() {
        assert(!def.get_name().empty());
        assert(!actors.empty());
    }

    static SceneDef fromDef(vulk::cpp2::SceneDef,
                            unordered_map<string, std::shared_ptr<PipelineDef>> const& pipelines,
                            unordered_map<string, shared_ptr<ModelDef>> const& models,
                            unordered_map<string, shared_ptr<MeshDef>> const& meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> const& materials);
};

// The metadata is valid up to the point of loading resources, but does
// not contain the resources themselves. The resources are loaded on demand.
struct Metadata {
    std::filesystem::path assetsDir;
    unordered_map<string, shared_ptr<MeshDef>> meshes;
    unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> vertShaders;
    unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> geometryShaders;
    unordered_map<string, shared_ptr<vulk::cpp2::ShaderDef>> fragmentShaders;
    unordered_map<string, shared_ptr<MaterialDef>> materials;
    unordered_map<string, shared_ptr<ModelDef>> models;
    unordered_map<string, shared_ptr<PipelineDef>> pipelines;
    unordered_map<string, shared_ptr<SceneDef>> scenes;
};

extern void findAndProcessMetadata(const fs::path path, Metadata& metadata);
extern std::shared_ptr<const Metadata> getMetadata();
// extern std::filesystem::path getResourcesDir();