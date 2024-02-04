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

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

struct MeshDef
{
    string name;
    fs::path path;
};

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

#define MODEL_JSON_VERSION 1
struct ModelDef
{
    string name;
    shared_ptr<MeshDef> mesh;
    shared_ptr<MaterialDef> material;

    void validate()
    {
        assert(!name.empty());
        assert(mesh);
        assert(material);
    }

    static ModelDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<MeshDef>> const &meshes, unordered_map<string, shared_ptr<MaterialDef>> materials)
    {
        ModelDef m;
        assert(j.at("version").get<uint32_t>() == MODEL_JSON_VERSION);
        m.name = j.at("name").get<string>();
        auto meshname = j.at("mesh").get<string>();
        m.mesh = meshes.at(meshname);
        m.material = materials.at(j.at("material").get<string>());
        m.validate();
        return m;
    }
};

#define ACTOR_JSON_VERSION 1
struct ActorDef
{
    string name;
    shared_ptr<PipelineDef> pipeline;
    shared_ptr<ModelDef> model;

    void validate()
    {
        assert(!name.empty());
        assert(pipeline);
        assert(model);
    }

    static ActorDef fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<PipelineDef>> const &pipelines, unordered_map<string, shared_ptr<ModelDef>> const &models)
    {
        ActorDef a;
        assert(j.at("version").get<uint32_t>() == ACTOR_JSON_VERSION);
        a.name = j.at("name").get<string>();
        a.pipeline = pipelines.at(j.at("pipeline").get<string>());
        a.model = models.at(j.at("model").get<string>());
        a.validate();
        return a;
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
    unordered_map<string, shared_ptr<ActorDef>> actors;
    unordered_map<string, shared_ptr<PipelineDef>> pipelines;
} metadata;

void findAndProcessMetadata(const fs::path &path)
{
    assert(fs::exists(path) && fs::is_directory(path));

    static set<string> extensionsToProcess{".model", ".actor", ".pipeline"};
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
            if (extensionsToProcess.find(ext) != extensionsToProcess.end())
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
                metadata.meshes[stem] = make_shared<MeshDef>(stem, entry.path());
            }
        }
    }

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

    for (auto const &[name, loadInfo] : loadInfos[".actor"])
    {
        auto actorDef = make_shared<ActorDef>(ActorDef::fromJSON(loadInfo.j, metadata.pipelines, metadata.models));
        assert(!metadata.actors.contains(actorDef->name));
        metadata.actors[actorDef->name] = actorDef;
    }
}
