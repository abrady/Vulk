// only to be included by VulkResources.cpp
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

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

namespace nlohmann
{
    template <>
    struct adl_serializer<glm::vec3>
    {
        static void to_json(json &j, const glm::vec3 &v)
        {
            j = json{v.x, v.y, v.z};
        }

        static void from_json(const json &j, glm::vec3 &v)
        {
            v.x = j.at(0).get<float>();
            v.y = j.at(1).get<float>();
            v.z = j.at(2).get<float>();
        }
    };
}

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

MaterialDef loadMaterialDef(const fs::path &file)
{
    if (!fs::exists(file))
    {
        throw std::runtime_error("Material file does not exist: " + file.string());
    }

    std::ifstream mtlFile(file);
    if (!mtlFile.is_open())
    {
        throw std::runtime_error("Failed to open material file: " + file.string());
    }

    MaterialDef material;
    std::string line;
    fs::path basePath = file.parent_path();
    bool startedNewMtl = false;
    while (std::getline(mtlFile, line))
    {
        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        auto processPath = [&](const std::string &relativePath) -> fs::path
        {
            fs::path absPath = fs::absolute(basePath / relativePath);
            if (!fs::exists(absPath))
            {
                throw std::runtime_error("Referenced file does not exist: " + absPath.string());
            }
            return absPath;
        };

        if (prefix == "newmtl")
        {
            assert(!startedNewMtl);
            startedNewMtl = true; // just handle 1 material for now
            string mtlName;
            lineStream >> mtlName;
            assert(mtlName == file.stem().string());
            material.name = mtlName;
        }
        else if (prefix == "map_Ka")
        {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKa = processPath(relativePath);
        }
        else if (prefix == "map_Kd")
        {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKd = processPath(relativePath);
        }
        else if (prefix == "map_Ks")
        {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKs = processPath(relativePath);
        }
        else if (prefix == "map_Bump")
        {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapNormal = processPath(relativePath);
        }
        else if (prefix == "Ns")
        {
            lineStream >> material.Ns;
        }
        else if (prefix == "Ni")
        {
            lineStream >> material.Ni;
        }
        else if (prefix == "d")
        {
            lineStream >> material.d;
        }
        else if (prefix == "Ka")
        {
            lineStream >> material.Ka[0] >> material.Ka[1] >> material.Ka[2];
        }
        else if (prefix == "Kd")
        {
            lineStream >> material.Kd[0] >> material.Kd[1] >> material.Kd[2];
        }
        else if (prefix == "Ks")
        {
            lineStream >> material.Ks[0] >> material.Ks[1] >> material.Ks[2];
        }
        // Add handling for other properties as needed
    }

    return material;
}

VkShaderStageFlagBits shaderStageFromStr(string const &stage)
{
    if (stage == "vertex")
    {
        return VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (stage == "fragment")
    {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else
    {
        throw runtime_error("Unknown shader stage: " + stage);
    }
}

VulkShaderUBOBindings shaderUBOBindingFromStr(string const &binding)
{
    static unordered_map<string, VulkShaderUBOBindings> bindings{
        {"xforms", VulkShaderUBOBinding_Xforms},
        {"lights", VulkShaderUBOBinding_Lights},
        {"eyePos", VulkShaderUBOBinding_EyePos},
        {"wavesXform", VulkShaderUBOBinding_WavesXform},
        {"modelXform", VulkShaderUBOBinding_ModelXform},
        {"mirrorPlaneUBO", VulkShaderUBOBinding_MirrorPlaneUBO},
        {"materialUBO", VulkShaderUBOBinding_MaterialUBO},
    };
    return bindings.at(binding);
    static_assert(VulkShaderUBOBinding_MaxBindingID == VulkShaderUBOBinding_MaterialUBO, "this function must be kept in sync with VulkShaderUBOBindings");
}

VulkShaderSSBOBindings shaderSSBOBindingFromStr(string const &binding)
{
    static unordered_map<string, VulkShaderSSBOBindings> bindings{
        {"actors", VulkShaderSSBOBinding_Actors},
        {"materials", VulkShaderSSBOBinding_Materials},
    };
    return bindings.at(binding);
    static_assert(VulkShaderSSBOBinding_MaxBindingID == VulkShaderSSBOBinding_Materials, "this function must be kept in sync with VulkShaderSSBOBindings");
}

VulkShaderTextureBindings shaderTextureBindingFromStr(string const &binding)
{
    static unordered_map<string, VulkShaderTextureBindings> bindings{
        {"textureSampler", VulkShaderTextureBinding_TextureSampler},
        {"textureSampler2", VulkShaderTextureBinding_TextureSampler2},
        {"textureSampler3", VulkShaderTextureBinding_TextureSampler3},
        {"normalSampler", VulkShaderTextureBinding_NormalSampler},
    };
    return bindings.at(binding);
    static_assert(VulkShaderTextureBinding_MaxBindingID == VulkShaderTextureBinding_NormalSampler, "this function must be kept in sync with VulkShaderTextureBindings");
}

struct DescriptorSetDef
{
    unordered_map<VulkShaderUBOBindings, VkShaderStageFlagBits> uniformBuffers;
    unordered_map<VulkShaderSSBOBindings, VkShaderStageFlagBits> storageBuffers;
    unordered_map<VulkShaderTextureBindings, VkShaderStageFlagBits> imageSamplers;

    void validate()
    {
        assert(uniformBuffers.size() + storageBuffers.size() + imageSamplers.size() > 0);
    }
};

void from_json(const nlohmann::json &j, DescriptorSetDef &ds)
{
    if (j.contains("uniformBuffers"))
    {
        unordered_map<string, string> uniformBuffers = j.at("uniformBuffers").get<unordered_map<string, string>>();
        for (auto const &[key, value] : uniformBuffers)
        {
            ds.uniformBuffers[shaderUBOBindingFromStr(key)] = shaderStageFromStr(value);
        }
    }
    if (j.contains("storageBuffers"))
    {
        unordered_map<string, string> storageBuffers = j.at("storageBuffers").get<unordered_map<string, string>>();
        for (auto const &[key, value] : storageBuffers)
        {
            ds.storageBuffers[shaderSSBOBindingFromStr(key)] = shaderStageFromStr(value);
        }
    }
    if (j.contains("imageSamplers"))
    {
        unordered_map<string, string> imageSamplers = j.at("imageSamplers").get<unordered_map<string, string>>();
        for (auto const &[key, value] : imageSamplers)
        {
            ds.imageSamplers[shaderTextureBindingFromStr(key)] = shaderStageFromStr(value);
        }
    }
}

#define PIPELINE_JSON_VERSION 1
struct PipelineDef
{
    string name;
    shared_ptr<ShaderDef> vertexShader;
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

    static PipelineDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<ShaderDef>> const &vertexShaders, unordered_map<string, shared_ptr<ShaderDef>> const &fragmentShaders)
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
        from_json(j.at("descriptorSet"), p.descriptorSet); // Use custom from_json for DescriptorSetDef
        p.validate();
        return p;
    }
};

enum MeshDefType
{
    MeshDefType_Model,
    MeshDefType_GeoMesh,
};
static unordered_map<string, MeshDefType> meshDefTypeMap{
    {"Model", MeshDefType_Model},
    {"GeoMesh", MeshDefType_GeoMesh},
};

enum GeoMeshDefType
{
    GeoMeshDefType_Sphere,
    GeoMeshDefType_Cylinder,
};
static unordered_map<string, GeoMeshDefType> geoMeshDefTypeMap{
    {"Sphere", GeoMeshDefType_Sphere},
    {"Cylinder", GeoMeshDefType_Cylinder},
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

    static ModelDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<MeshDef>> const &meshes, unordered_map<string, shared_ptr<MaterialDef>> materials)
    {
        assert(j.at("version").get<uint32_t>() == MODEL_JSON_VERSION);
        auto name = j.at("name").get<string>();
        auto material = materials.at(j.at("material").get<string>());
        MeshDefType meshDefType = j.contains("type") ? meshDefTypeMap.at(j.at("type").get<string>()) : MeshDefType_Model;
        switch (meshDefType)
        {
        case MeshDefType_Model:
            return ModelDef(name, meshes.at(j.at("mesh").get<string>()), material);
        case MeshDefType_GeoMesh:
        {
            shared_ptr<MeshDef> mesh;
            auto meshJson = j.at("GeoMesh");
            GeoMeshDefType type = geoMeshDefTypeMap.at(meshJson.at("type").get<string>());
            switch (type)
            {
            case GeoMeshDefType_Sphere:
            {
                GeoMeshDef::Sphere sphere;
                sphere.radius = meshJson.at("radius").get<float>();
                sphere.numSubdivisions = meshJson.at("numSubdivisions").get<uint32_t>();
                mesh = make_shared<MeshDef>(name + ".GeoMesh.Sphere", sphere);
            }
            break;
            case GeoMeshDefType_Cylinder:
            {
                GeoMeshDef::Cylinder cylinder;
                cylinder.height = meshJson.at("height").get<float>();
                cylinder.bottomRadius = meshJson.at("bottomRadius").get<float>();
                cylinder.topRadius = meshJson.at("topRadius").get<float>();
                cylinder.numStacks = meshJson.at("numStacks").get<uint32_t>();
                cylinder.numSlices = meshJson.at("numSlices").get<uint32_t>();

                mesh = make_shared<MeshDef>(name + ".GeoMesh.Cylinder", cylinder);
            }
            break;
            default:
                throw runtime_error("Unknown GeoMesh type: " + type);
            }
            return ModelDef(name, mesh, material);
        }
        default:
            throw runtime_error("Unknown MeshDef type: " + meshDefType);
        };
    }
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
        unordered_map<string, shared_ptr<MaterialDef>> materials)
    {
        ActorDef a;
        a.name = j.at("name").get<string>();
        a.pipeline = pipelines.at(j.at("pipeline").get<string>());

        if (j.contains("model"))
        {
            assert(!j.contains("inlineModel"));
            a.model = models.at(j.at("model").get<string>());
        }
        else if (j.contains("inlineModel"))
        {
            a.model = make_shared<ModelDef>(ModelDef::fromJSON(j.at("inlineModel"), meshes, materials));
        }
        else
        {
            throw runtime_error("ActorDef must contain either a model or an inlineModel");
        }

        // make the transform
        glm::mat4 xform = glm::mat4(1.0f);
        if (j.contains("xform"))
        {
            auto jx = j.at("xform");

            glm::vec3 pos = glm::vec3(0);
            glm::vec3 rot = glm::vec3(0);
            glm::vec3 scale = glm::vec3(1);
            if (jx.contains("position"))
                pos = jx.at("position").get<glm::vec3>();
            if (jx.contains("rotationYPR"))
                rot = jx.at("rotationYPR").get<glm::vec3>();
            if (jx.contains("scale"))
                scale = jx.at("scale").get<glm::vec3>();
            xform = glm::translate(glm::mat4(1.0f), pos) * glm::yawPitchRoll(rot.y, rot.x, rot.z) * glm::scale(glm::mat4(1.0f), scale);
        }
        a.xform = xform;
        a.validate();
        return a;
    }
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
        unordered_map<string, shared_ptr<MaterialDef>> const &materials)
    {
        SceneDef s;
        assert(j.at("version").get<uint32_t>() == SCENE_JSON_VERSION);
        s.name = j.at("name").get<string>();

        // load the camera
        auto jcam = j.at("camera");
        glm::vec3 eye = jcam.at("eye").get<glm::vec3>();
        glm::vec3 target = jcam.at("target").get<glm::vec3>();
        s.camera.lookAt(eye, target);

        // load the lights
        for (auto const &light : j.at("lights").get<vector<json>>())
        {
            string type = light.at("type").get<string>();
            if (type == "point")
            {
                auto pos = light.at("pos").get<glm::vec3>();
                auto color = light.at("color").get<glm::vec3>();
                auto falloffStart = light.contains("falloffStart") ? light.at("falloffStart").get<float>() : 0.f;
                auto falloffEnd = light.contains("falloffEnd") ? light.at("falloffEnd").get<float>() : 0.f;
                s.pointLights.push_back(make_shared<VulkPointLight>(pos, falloffStart, color, falloffEnd));
            }
            else
            {
                throw runtime_error("Unknown light type: " + type);
            }
        };

        // load the actors
        for (auto const &actor : j.at("actors").get<vector<nlohmann::json>>())
        {
            auto a = make_shared<ActorDef>(ActorDef::fromJSON(actor, pipelines, models, meshes, materials));
            s.actors.push_back(a);
            assert(!s.actorMap.contains(a->name));
            s.actorMap[a->name] = a;
        }
        s.validate();
        return s;
    }
};

// The metadata is valid up to the point of loading resources, but does
// not contain the resources themselves. The resources are loaded on demand.
struct Metadata
{
    unordered_map<string, shared_ptr<MeshDef>> meshes;
    unordered_map<string, shared_ptr<ShaderDef>> vertexShaders;
    unordered_map<string, shared_ptr<ShaderDef>> fragmentShaders;
    unordered_map<string, shared_ptr<MaterialDef>> materials;
    unordered_map<string, shared_ptr<ModelDef>> models;
    unordered_map<string, shared_ptr<PipelineDef>> pipelines;
    unordered_map<string, shared_ptr<SceneDef>> scenes;
} metadata;

void findAndProcessMetadata(const fs::path &path)
{
    assert(fs::exists(path) && fs::is_directory(path));

    // The metadata is stored in JSON files with the following extensions
    // other extensions need special handling or no handling (e.g. .mtl files are handled by
    // loadMaterialDef, and .obj and .spv files are handled directly)
    static set<string> jsonExts{".model", ".pipeline", ".scene"};
    struct LoadInfo
    {
        json j;
        fs::path path;
    };
    unordered_map<string, unordered_map<string, LoadInfo>> loadInfos;

    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            string stem = entry.path().stem().string();
            string ext = entry.path().extension().string();
            if (jsonExts.find(ext) != jsonExts.end())
            {
                ifstream f(entry.path());
                LoadInfo loadInfo;
                loadInfo.j = nlohmann::json::parse(f);
                loadInfo.path = entry.path().parent_path();
                assert(loadInfo.j.at("name") == stem);
                loadInfos[ext][loadInfo.j.at("name")] = loadInfo;
            }
            else if (ext == ".vertspv")
            {
                assert(!metadata.vertexShaders.contains(stem));
                metadata.vertexShaders[stem] = make_shared<ShaderDef>(stem, entry.path());
            }
            else if (ext == ".fragspv")
            {
                assert(!metadata.fragmentShaders.contains(stem));
                metadata.fragmentShaders[stem] = make_shared<ShaderDef>(stem, entry.path());
            }
            else if (ext == ".mtl")
            {
                assert(!metadata.materials.contains(stem));
                auto material = make_shared<MaterialDef>(loadMaterialDef(entry.path()));
                metadata.materials[material->name] = material;
            }
            else if (ext == ".obj")
            {
                assert(!metadata.meshes.contains(stem));
                ModelMeshDef mmd{entry.path()};
                metadata.meshes[stem] = make_shared<MeshDef>(stem, mmd);
            }
        }
    }

    // The order matters here: models depend on meshes and materials, actors depend on models and pipelines
    // and the scene depends on actors

    for (auto const &[name, loadInfo] : loadInfos[".pipeline"])
    {
        auto pipelineDef = make_shared<PipelineDef>(PipelineDef::fromJSON(loadInfo.j, metadata.vertexShaders, metadata.fragmentShaders));
        assert(!metadata.pipelines.contains(pipelineDef->name));
        metadata.pipelines[pipelineDef->name] = pipelineDef;
    }

    for (auto const &[name, loadInfo] : loadInfos[".model"])
    {
        auto modelDef = make_shared<ModelDef>(ModelDef::fromJSON(loadInfo.j, metadata.meshes, metadata.materials));
        assert(!metadata.models.contains(modelDef->name));
        metadata.models[modelDef->name] = modelDef;
    }

    for (auto const &[name, loadInfo] : loadInfos[".scene"])
    {
        auto sceneDef = make_shared<SceneDef>(SceneDef::fromJSON(loadInfo.j, metadata.pipelines, metadata.models, metadata.meshes, metadata.materials));
        assert(!metadata.scenes.contains(sceneDef->name));
        metadata.scenes[sceneDef->name] = sceneDef;
    }
}
