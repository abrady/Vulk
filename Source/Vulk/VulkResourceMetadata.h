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

#include "VulkGeo.h"
#include "VulkCamera.h"
#include "VulkPointLight.h"
#include "VulkModel.h"
#include "VulkScene.h"
#include "VulkShaderModule.h"

using namespace std;
namespace fs = std::filesystem;

struct ShaderDef
{
    string name;
    fs::path path;
};

struct MaterialDef
{
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
    MaterialDef() : Ns(0.0f), Ni(1.0f), d(1.0f), Ka{0.0f, 0.0f, 0.0f}, Kd{0.0f, 0.0f, 0.0f}, Ks{0.0f, 0.0f, 0.0f} {}

    VulkMaterialConstants toVulkMaterialConstants()
    {
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

struct DescriptorSetDef
{
    unordered_map<VulkShaderUBOBindings, VkShaderStageFlagBits> uniformBuffers;
    unordered_map<VulkShaderSSBOBindings, VkShaderStageFlagBits> storageBuffers;
    unordered_map<VulkShaderTextureBindings, VkShaderStageFlagBits> imageSamplers;

    void validate() { assert(uniformBuffers.size() + storageBuffers.size() + imageSamplers.size() > 0); }
    static DescriptorSetDef fromJSON(const nlohmann::json &j);
};

#define PIPELINE_JSON_VERSION 1
struct PipelineDef
{
    string name;
    shared_ptr<ShaderDef> vertexShader;
    shared_ptr<ShaderDef> geometryShader;
    shared_ptr<ShaderDef> fragmentShader;
    uint32_t vertexInputBinding;
    DescriptorSetDef descriptorSet;

    void validate()
    {
        assert(!name.empty());
        assert(vertexShader);
        assert(fragmentShader);
        descriptorSet.validate();
    }

    static PipelineDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<ShaderDef>> const &vertexShaders, unordered_map<string, shared_ptr<ShaderDef>> const &geometryShaders, unordered_map<string, shared_ptr<ShaderDef>> const &fragmentShaders)
    {
        PipelineDef p;
        assert(j.at("version").get<uint32_t>() == PIPELINE_JSON_VERSION);
        p.name = j.at("name").get<string>();
        auto shader = j.at("vertexShader").get<string>();
        assert(vertexShaders.contains(shader));
        p.vertexShader = vertexShaders.at(shader);
        shader = j.at("fragmentShader").get<string>();
        assert(fragmentShaders.contains(shader));
        p.fragmentShader = fragmentShaders.at(shader);
        p.vertexInputBinding = j.at("vertexInputBinding").get<uint32_t>();
        p.descriptorSet = DescriptorSetDef::fromJSON(j.at("descriptorSet")); // Use custom from_json for DescriptorSetDef

        if (j.contains("geometryShader"))
        {
            shader = j.at("geometryShader").get<string>();
            assert(geometryShaders.contains(shader));
            p.geometryShader = geometryShaders.at(shader);
        }
        p.validate();
        return p;
    }
};

enum MeshDefType
{
    MeshDefType_Model,
    MeshDefType_GeoMesh,
};

enum GeoMeshDefType
{
    GeoMeshDefType_Sphere,
    GeoMeshDefType_Cylinder,
};

struct ModelMeshDef
{
    fs::path path;
};

struct GeoMeshDef
{
    struct Sphere
    {
        float radius;
        uint32_t numSubdivisions;
    };
    struct Cylinder
    {
        float height;
        float bottomRadius;
        float topRadius;
        uint32_t numStacks;
        uint32_t numSlices;
    };
    GeoMeshDefType type;
    union
    {
        Sphere sphere;
        Cylinder cylinder;
    };
    GeoMeshDef(Sphere sphere) : type(GeoMeshDefType_Sphere), sphere(sphere){};
    GeoMeshDef(Cylinder cylinder) : type(GeoMeshDefType_Cylinder), cylinder(cylinder){};
};

struct MeshDef
{
    string name;
    MeshDefType type;
    MeshDef(string name, ModelMeshDef model) : name(name), type(MeshDefType_Model), model(make_shared<ModelMeshDef>(model)){};
    MeshDef(string name, GeoMeshDef geo) : name(name), type(MeshDefType_GeoMesh), geo(make_shared<GeoMeshDef>(geo)){};
    shared_ptr<ModelMeshDef> getModel()
    {
        assert(type == MeshDefType_Model);
        return model;
    }
    shared_ptr<GeoMeshDef> getGeoMesh()
    {
        assert(type == MeshDefType_GeoMesh);
        return geo;
    }

private:
    shared_ptr<ModelMeshDef> model;
    shared_ptr<GeoMeshDef> geo;
};

#define MODEL_JSON_VERSION 1
struct ModelDef
{
    string name;
    shared_ptr<MeshDef> mesh;
    shared_ptr<MaterialDef> material;

    ModelDef(string name, shared_ptr<MeshDef> mesh, shared_ptr<MaterialDef> material) : name(name), mesh(mesh), material(material)
    {
        assert(!name.empty());
        assert(mesh);
    }

    static ModelDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<MeshDef>> const &meshes, unordered_map<string, shared_ptr<MaterialDef>> materials);
};

struct ActorDef
{
    string name;
    shared_ptr<PipelineDef> pipeline;
    shared_ptr<ModelDef> model;
    glm::mat4 xform = glm::mat4(1.0f);

    void validate()
    {
        assert(!name.empty());
        assert(pipeline);
        assert(model);
        assert(xform != glm::mat4(0.0f));
    }

    static ActorDef fromJSON(
        const nlohmann::json &j,
        unordered_map<string, shared_ptr<PipelineDef>> const &pipelines,
        unordered_map<string, shared_ptr<ModelDef>> const &models,
        unordered_map<string, shared_ptr<MeshDef>> meshes,
        unordered_map<string, shared_ptr<MaterialDef>> materials);
};

#define SCENE_JSON_VERSION 1
struct SceneDef
{
    string name;
    VulkCamera camera;
    vector<shared_ptr<VulkPointLight>> pointLights;
    vector<shared_ptr<ActorDef>> actors;
    unordered_map<string, shared_ptr<ActorDef>> actorMap;

    void validate()
    {
        assert(!name.empty());
        assert(!actors.empty());
    }

    static SceneDef fromJSON(
        const nlohmann::json &j,
        unordered_map<string, shared_ptr<PipelineDef>> const &pipelines,
        unordered_map<string, shared_ptr<ModelDef>> const &models,
        unordered_map<string, shared_ptr<MeshDef>> const &meshes,
        unordered_map<string, shared_ptr<MaterialDef>> const &materials);
};

// The metadata is valid up to the point of loading resources, but does
// not contain the resources themselves. The resources are loaded on demand.
struct Metadata
{
    unordered_map<string, shared_ptr<MeshDef>> meshes;
    unordered_map<string, shared_ptr<ShaderDef>> vertexShaders;
    unordered_map<string, shared_ptr<ShaderDef>> geometryShaders;
    unordered_map<string, shared_ptr<ShaderDef>> fragmentShaders;
    unordered_map<string, shared_ptr<MaterialDef>> materials;
    unordered_map<string, shared_ptr<ModelDef>> models;
    unordered_map<string, shared_ptr<PipelineDef>> pipelines;
    unordered_map<string, shared_ptr<SceneDef>> scenes;
};

extern Metadata const *getMetadata();
