#include "VulkResourceMetadata.h"

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

namespace nlohmann {
    template <> struct adl_serializer<glm::vec3> {
        static void to_json(json &j, const glm::vec3 &v) {
            j = json{v.x, v.y, v.z};
        }

        static void from_json(const json &j, glm::vec3 &v) {
            v.x = j.at(0).get<float>();
            v.y = j.at(1).get<float>();
            v.z = j.at(2).get<float>();
        }
    };
} // namespace nlohmann

MaterialDef loadMaterialDef(const fs::path &file) {
    if (!fs::exists(file)) {
        throw std::runtime_error("Material file does not exist: " + file.string());
    }

    std::ifstream mtlFile(file);
    if (!mtlFile.is_open()) {
        throw std::runtime_error("Failed to open material file: " + file.string());
    }

    MaterialDef material;
    std::string line;
    fs::path basePath = file.parent_path();
    bool startedNewMtl = false;
    while (std::getline(mtlFile, line)) {
        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        auto processPath = [&](const std::string &relativePath) -> fs::path {
            fs::path absPath = fs::absolute(basePath / relativePath);
            if (!fs::exists(absPath)) {
                throw std::runtime_error("Referenced file does not exist: " + absPath.string());
            }
            return absPath;
        };

        if (prefix == "newmtl") {
            assert(!startedNewMtl);
            startedNewMtl = true; // just handle 1 material for now
            string mtlName;
            lineStream >> mtlName;
            assert(mtlName == file.stem().string());
            material.name = mtlName;
        } else if (prefix == "map_Ka") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKa = processPath(relativePath);
        } else if (prefix == "map_Kd") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKd = processPath(relativePath);
        } else if (prefix == "map_Ks") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKs = processPath(relativePath);
        } else if (prefix == "map_Bump") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapNormal = processPath(relativePath);
        } else if (prefix == "Ns") {
            lineStream >> material.Ns;
        } else if (prefix == "Ni") {
            lineStream >> material.Ni;
        } else if (prefix == "d") {
            lineStream >> material.d;
        } else if (prefix == "Ka") {
            lineStream >> material.Ka[0] >> material.Ka[1] >> material.Ka[2];
        } else if (prefix == "Kd") {
            lineStream >> material.Kd[0] >> material.Kd[1] >> material.Kd[2];
        } else if (prefix == "Ks") {
            lineStream >> material.Ks[0] >> material.Ks[1] >> material.Ks[2];
        }
        // Add handling for other properties as needed
    }

    return material;
}

string shaderUBOBindingToStr(VulkShaderUBOBindings binding) {
    static unordered_map<VulkShaderUBOBindings, string> bindings;
    static once_flag flag;
    call_once(flag, [&]() {
        for (auto const &[name, value] : getUBOBindings()) {
            assert(!bindings.contains(value));
            bindings[value] = name;
        }
    });
    return bindings.at(binding);
}

string shaderTextureBindingToStr(VulkShaderTextureBindings binding) {
    static unordered_map<VulkShaderTextureBindings, string> bindings;
    static once_flag flag;
    call_once(flag, [&]() {
        for (auto const &[name, value] : getTextureBindings()) {
            assert(!bindings.contains(value));
            bindings[value] = name;
        }
    });
    return bindings.at(binding);
}

static unordered_map<string, MeshDefType> meshDefTypeMap{
    {"Model", MeshDefType_Model},
    {"Mesh", MeshDefType_Mesh},
};

enum GeoMeshDefType {
    GeoMeshDefType_Sphere,
    GeoMeshDefType_Cylinder,
    GeoMeshDefType_EquilateralTriangle,
    GeoMeshDefType_Quad,
    GeoMeshDefType_Grid,
    GeoMeshDefType_Axes,
};
static unordered_map<string, GeoMeshDefType> geoMeshDefTypeMap{
    {"Sphere", GeoMeshDefType_Sphere}, {"Cylinder", GeoMeshDefType_Cylinder}, {"Triangle", GeoMeshDefType_EquilateralTriangle},
    {"Quad", GeoMeshDefType_Quad},     {"Grid", GeoMeshDefType_Grid},         {"Axes", GeoMeshDefType_Axes},
};

ModelDef ModelDef::fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<MeshDef>> const &meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> materials) {
    assert(j.at("version").get<uint32_t>() == MODEL_JSON_VERSION);
    auto name = j.at("name").get<string>();
    auto material = materials.at(j.at("material").get<string>());
    MeshDefType meshDefType = j.contains("type") ? meshDefTypeMap.at(j.at("type").get<string>()) : MeshDefType_Model;
    switch (meshDefType) {
    case MeshDefType_Model:
        return ModelDef(name, meshes.at(j.at("mesh").get<string>()), material);
    case MeshDefType_Mesh: {
        shared_ptr<VulkMesh> mesh = make_shared<VulkMesh>();
        auto meshJson = j.at("GeoMesh");
        GeoMeshDefType type = geoMeshDefTypeMap.at(meshJson.at("type").get<string>());
        switch (type) {
        case GeoMeshDefType_Sphere: {
            float radius = meshJson.at("radius").get<float>();
            uint32_t numSubdivisions = meshJson.value("numSubdivisions", 0);
            mesh = make_shared<VulkMesh>();
            makeGeoSphere(radius, numSubdivisions, *mesh);
        } break;
        case GeoMeshDefType_Cylinder: {
            float height = meshJson.at("height").get<float>();
            float bottomRadius = meshJson.at("bottomRadius").get<float>();
            float topRadius = meshJson.at("topRadius").get<float>();
            uint32_t numStacks = meshJson.at("numStacks").get<uint32_t>();
            uint32_t numSlices = meshJson.at("numSlices").get<uint32_t>();
            makeCylinder(height, bottomRadius, topRadius, numStacks, numSlices, *mesh);
        } break;
        case GeoMeshDefType_EquilateralTriangle: {
            float side = meshJson.at("side").get<float>();
            uint32_t numSubdivisions = meshJson.at("numSubdivisions").get<uint32_t>();
            makeEquilateralTri(side, numSubdivisions, *mesh);
        } break;
        case GeoMeshDefType_Quad: {
            float w = meshJson.at("width").get<float>();
            float h = meshJson.at("height").get<float>();
            uint32_t numSubdivisions = meshJson.at("numSubdivisions").get<uint32_t>();
            makeQuad(w, h, numSubdivisions, *mesh);
        } break;
        case GeoMeshDefType_Grid: {
            float width = meshJson.at("width").get<float>();
            float depth = meshJson.at("depth").get<float>();
            uint32_t m = meshJson.at("m").get<uint32_t>();
            uint32_t n = meshJson.at("n").get<uint32_t>();
            float repeatU = meshJson.value("repeatU", 1.0f);
            float repeatV = meshJson.value("repeatV", 1.0f);
            makeGrid(width, depth, m, n, *mesh, repeatU, repeatV);
        } break;
        case GeoMeshDefType_Axes: {
            float length = meshJson.at("length").get<float>();
            makeAxes(length, *mesh);
        } break;
        default:
            throw runtime_error("Unknown GeoMesh type: " + type);
        }
        return ModelDef(name, make_shared<MeshDef>(name, mesh), material);
    }
    default:
        throw runtime_error("Unknown MeshDef type: " + meshDefType);
    };
}

ActorDef ActorDef::fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<PipelineDef>> const &pipelines,
                            unordered_map<string, shared_ptr<ModelDef>> const &models, unordered_map<string, shared_ptr<MeshDef>> meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> materials) {
    ActorDef a;
    a.name = j.at("name").get<string>();
    a.pipeline = pipelines.at(j.at("pipeline").get<string>());

    if (j.contains("model")) {
        assert(!j.contains("inlineModel"));
        a.model = models.at(j.at("model").get<string>());
    } else if (j.contains("inlineModel")) {
        a.model = make_shared<ModelDef>(ModelDef::fromJSON(j.at("inlineModel"), meshes, materials));
    } else {
        throw runtime_error("ActorDef must contain either a model or an inlineModel");
    }

    // make the transform
    glm::mat4 xform = glm::mat4(1.0f);
    if (j.contains("xform")) {
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

SceneDef SceneDef::fromJSON(const nlohmann::json &j, unordered_map<string, shared_ptr<PipelineDef>> const &pipelines,
                            unordered_map<string, shared_ptr<ModelDef>> const &models, unordered_map<string, shared_ptr<MeshDef>> const &meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> const &materials) {
    SceneDef s;
    assert(j.at("version").get<uint32_t>() == SCENE_JSON_VERSION);
    s.name = j.at("name").get<string>();

    // load the camera
    auto jcam = j.at("camera");
    glm::vec3 eye = jcam.at("eye").get<glm::vec3>();
    glm::vec3 target = jcam.at("target").get<glm::vec3>();
    s.camera.lookAt(eye, target);

    // load the lights
    for (auto const &light : j.at("lights").get<vector<json>>()) {
        string type = light.at("type").get<string>();
        if (type == "point") {
            auto pos = light.at("pos").get<glm::vec3>();
            auto color = light.at("color").get<glm::vec3>();
            auto falloffStart = light.contains("falloffStart") ? light.at("falloffStart").get<float>() : 0.f;
            auto falloffEnd = light.contains("falloffEnd") ? light.at("falloffEnd").get<float>() : 0.f;
            s.pointLights.push_back(make_shared<VulkPointLight>(pos, falloffStart, color, falloffEnd));
        } else {
            throw runtime_error("Unknown light type: " + type);
        }
    };

    // load the actors
    for (auto const &actor : j.at("actors").get<vector<nlohmann::json>>()) {
        auto a = make_shared<ActorDef>(ActorDef::fromJSON(actor, pipelines, models, meshes, materials));
        s.actors.push_back(a);
        assert(!s.actorMap.contains(a->name));
        s.actorMap[a->name] = a;
    }
    s.validate();
    return s;
}

void findAndProcessMetadata(const fs::path &path, Metadata &metadata) {
    assert(fs::exists(path) && fs::is_directory(path));

    // The metadata is stored in JSON files with the following extensions
    // other extensions need special handling or no handling (e.g. .mtl files are handled by
    // loadMaterialDef, and .obj and .spv files are handled directly)
    static set<string> jsonExts{".model", ".pipeline", ".scene"};
    struct LoadInfo {
        json j;
        fs::path path;
    };
    unordered_map<string, unordered_map<string, LoadInfo>> loadInfos;

    for (const auto &entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            string stem = entry.path().stem().string();
            string ext = entry.path().extension().string();
            if (jsonExts.find(ext) != jsonExts.end()) {
                ifstream f(entry.path());
                LoadInfo loadInfo;
                loadInfo.j = nlohmann::json::parse(f, nullptr, true, true); // allow comments
                loadInfo.path = entry.path().parent_path();
                assert(loadInfo.j.at("name") == stem);
                loadInfos[ext][loadInfo.j.at("name")] = loadInfo;
            } else if (ext == ".vertspv") {
                assert(!metadata.vertexShaders.contains(stem));
                metadata.vertexShaders[stem] = make_shared<ShaderDef>(stem, entry.path());
            } else if (ext == ".geomspv") {
                assert(!metadata.geometryShaders.contains(stem));
                metadata.geometryShaders[stem] = make_shared<ShaderDef>(stem, entry.path());
            } else if (ext == ".fragspv") {
                assert(!metadata.fragmentShaders.contains(stem));
                metadata.fragmentShaders[stem] = make_shared<ShaderDef>(stem, entry.path());
            } else if (ext == ".mtl") {
                assert(!metadata.materials.contains(stem));
                auto material = make_shared<MaterialDef>(loadMaterialDef(entry.path()));
                metadata.materials[material->name] = material;
            } else if (ext == ".obj") {
                assert(!metadata.meshes.contains(stem));
                ModelMeshDef mmd{entry.path()};
                metadata.meshes[stem] = make_shared<MeshDef>(stem, mmd);
            }
        }
    }

    // The order matters here: models depend on meshes and materials, actors depend on models and pipelines
    // and the scene depends on actors

    for (auto const &[name, loadInfo] : loadInfos[".pipeline"]) {
        auto pipelineDef =
            make_shared<PipelineDef>(PipelineDef::fromJSON(loadInfo.j, metadata.vertexShaders, metadata.geometryShaders, metadata.fragmentShaders));
        assert(!metadata.pipelines.contains(pipelineDef->name));
        metadata.pipelines[pipelineDef->name] = pipelineDef;
    }

    for (auto const &[name, loadInfo] : loadInfos[".model"]) {
        auto modelDef = make_shared<ModelDef>(ModelDef::fromJSON(loadInfo.j, metadata.meshes, metadata.materials));
        assert(!metadata.models.contains(modelDef->name));
        metadata.models[modelDef->name] = modelDef;
    }

    for (auto const &[name, loadInfo] : loadInfos[".scene"]) {
        auto sceneDef = make_shared<SceneDef>(SceneDef::fromJSON(loadInfo.j, metadata.pipelines, metadata.models, metadata.meshes, metadata.materials));
        assert(!metadata.scenes.contains(sceneDef->name));
        metadata.scenes[sceneDef->name] = sceneDef;
    }
}

Metadata const *getMetadata() {
    static Metadata metadata;
    static once_flag flag;
    call_once(flag, [&]() { findAndProcessMetadata(fs::current_path(), metadata); });

    return &metadata;
}
