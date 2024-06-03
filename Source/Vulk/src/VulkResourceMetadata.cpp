#include "Vulk/VulkResourceMetadata.h"
#include "Vulk/VulkEnumMetadata.h"

using json = nlohmann::json;
using namespace std;

namespace fs = std::filesystem;

namespace nlohmann {
template <>
struct adl_serializer<glm::vec3> {
    static void to_json(json& j, const glm::vec3& v) {
        j = json{v.x, v.y, v.z};
    }

    static void from_json(const json& j, glm::vec3& v) {
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
        v.z = j.at(2).get<float>();
    }
};
} // namespace nlohmann

MaterialDef loadMaterialDef(const fs::path& file) {
    if (!fs::exists(file)) {
        VULK_THROW("Material file does not exist: " + file.string());
    }

    std::ifstream mtlFile(file);
    if (!mtlFile.is_open()) {
        VULK_THROW("Failed to open material file: " + file.string());
    }

    // Ka: Ambient reflectivity
    // Kd: Diffuse reflectivity
    // Ks: Specular reflectivity
    // Ke: Emissive intensity
    // Ns: Specular exponent (controls the size and sharpness of specular highlights)
    // illum: Illumination model (number representing how many light sources are reflected/refracted)
    // map_Kd: Diffuse texture map
    // map_Ka: Ambient texture map
    // map_Ks: Specular texture map
    // map_Ns: Specular exponent texture map
    // map_d: Bump mapping texture
    // map_bump: Normal mapping texture
    // disp: Displacement texture
    // decal: Decal texture
    // refl: Reflection texture
    // trans: Transparency texture
    // alpha: Opacity value (1.0 being fully opaque, 0.0 fully transparent)
    // ambient: Ambient color
    // diffuse: Diffuse color
    // specular: Specular color
    // emission: Emissive color
    // transmission: Transmission filter color
    // backfacing: Controls whether the material is double-sided or not (on or off)
    // shadow: Controls whether the material casts shadows (on or off)
    // reflect: Controls whether the material reflects other objects (on or off)
    // refract: Controls whether the material refracts other objects (on or off)
    // ior: Index of refraction (for refraction calculations)
    // ior_threshold: Threshold for refraction (used when ior is non-zero)
    // ior_tint: Tint color for refracted objects (when ior is non-zero)

    MaterialDef material;
    std::string line;
    fs::path basePath = file.parent_path();
    bool startedNewMtl = false;
    while (std::getline(mtlFile, line)) {
        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        auto processPath = [&](const std::string& relativePath) -> std::string {
            fs::path absPath = fs::absolute(basePath / relativePath);
            if (!fs::exists(absPath)) {
                VULK_THROW("Referenced file does not exist: " + absPath.string());
            }
            return absPath.string();
        };

        if (prefix == "#" || prefix == "") {
            continue;
        } else if (prefix == "newmtl") {
            assert(!startedNewMtl);
            startedNewMtl = true; // just handle 1 material for now
            string mtlName;
            lineStream >> mtlName;
            assert(mtlName == file.stem().string());
            material.name = mtlName;
        } else if (prefix == "map_Ka") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKa = processPath(relativePath); // ambient texture
        } else if (prefix == "map_Kd") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKd = processPath(relativePath); // diffuse texture
        } else if (prefix == "map_Ks") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKs = processPath(relativePath); // specular texture
        } else if (prefix == "map_Bump" || prefix == "norm") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapNormal = processPath(relativePath); // normal map texture
        } else if (prefix == "map_Pm") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapPm = processPath(relativePath); // metalness map texture
        } else if (prefix == "map_Pr") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapPr = processPath(relativePath); // roughness map texture
        } else if (prefix == "disp") {
            std::string relativePath;
            lineStream >> relativePath;
            material.disp = processPath(relativePath); // displacement map texture
        } else if (prefix == "Ns") {
            lineStream >> material.Ns; // specular exponent
        } else if (prefix == "Ni") {
            lineStream >> material.Ni; // index of refraction
        } else if (prefix == "d" || prefix == "Tr") {
            lineStream >> material.d; // dissolve (transparency)
        } else if (prefix == "Ka") {
            lineStream >> material.Ka[0] >> material.Ka[1] >> material.Ka[2]; // ambient color
        } else if (prefix == "Kd") {
            lineStream >> material.Kd[0] >> material.Kd[1] >> material.Kd[2]; // diffuse color
        } else if (prefix == "Ks") {
            lineStream >> material.Ks[0] >> material.Ks[1] >> material.Ks[2]; // specular color
        } else {
            VULK_THROW_FMT("Unknown material property: {}", prefix);
        }
        // Add handling for other properties as needed
    }

    return material;
}

ModelDef ModelDef::fromJSON(const nlohmann::json& j, unordered_map<string, shared_ptr<MeshDef>> const& meshes, unordered_map<string, shared_ptr<MaterialDef>> materials) {
    assert(j.at("version").get<uint32_t>() == MODEL_JSON_VERSION);
    auto name = j.at("name").get<string>();
    string mn = j.at("material").get<string>();
    auto material = materials.at(mn);
    MeshDefType meshDefType = EnumLookup<MeshDefType>::getEnumFromStr(j.value("type", "Model"));
    switch (meshDefType) {
    case MeshDefType_Model:
        return ModelDef(name, meshes.at(j.at("mesh").get<string>()), material);
    case MeshDefType_Mesh: {
        shared_ptr<VulkMesh> mesh = make_shared<VulkMesh>();
        auto meshJson = j.at("GeoMesh");
        GeoMeshDefType type = EnumLookup<GeoMeshDefType>::getEnumFromStr(meshJson.at("type").get<string>());
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
            uint32_t numSubdivisions = meshJson.value("numSubdivisions", 0);
            makeEquilateralTri(side, numSubdivisions, *mesh);
        } break;
        case GeoMeshDefType_Quad: {
            float w = meshJson.at("width").get<float>();
            float h = meshJson.at("height").get<float>();
            uint32_t numSubdivisions = meshJson.value("numSubdivisions", 0);
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
            VULK_THROW_FMT("Unknown GeoMesh type: {}", type);
        }
        return ModelDef(name, make_shared<MeshDef>(name, mesh), material);
    }
    default:
        VULK_THROW_FMT("Unknown MeshDef type: {}", meshDefType);
    };
}

ActorDef ActorDef::fromJSON(const nlohmann::json& j, unordered_map<string, shared_ptr<BuiltPipelineDef>> const& pipelines,
                            unordered_map<string, shared_ptr<ModelDef>> const& models, unordered_map<string, shared_ptr<MeshDef>> meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> materials) {
    ActorDef a;
    a.name = j.at("name").get<string>();
    string pipelineName = j.at("pipeline").get<string>();
    a.pipeline = pipelines.at(pipelineName);

    if (j.contains("model")) {
        assert(!j.contains("inlineModel"));
        a.model = models.at(j.at("model").get<string>());
    } else if (j.contains("inlineModel")) {
        a.model = make_shared<ModelDef>(ModelDef::fromJSON(j.at("inlineModel"), meshes, materials));
    } else {
        VULK_THROW("ActorDef must contain either a model or an inlineModel");
    }

    // make the transform
    glm::mat4 xform = glm::mat4(1.0f);
    if (j.contains("xform")) {
        auto jx = j.at("xform");

        glm::vec3 pos = jx.value("pos", glm::vec3(0));
        glm::vec3 rot = glm::radians(jx.value("rot", glm::vec3(0)));
        glm::vec3 scale = jx.value("scale", glm::vec3(1));
        xform = glm::translate(glm::mat4(1.0f), pos) * glm::eulerAngleXYZ(rot.x, rot.y, rot.z) * glm::scale(glm::mat4(1.0f), scale);
    }
    a.xform = xform;
    a.validate();
    return a;
}

SceneDef SceneDef::fromJSON(const nlohmann::json& j, unordered_map<string, shared_ptr<BuiltPipelineDef>> const& pipelines,
                            unordered_map<string, shared_ptr<ModelDef>> const& models, unordered_map<string, shared_ptr<MeshDef>> const& meshes,
                            unordered_map<string, shared_ptr<MaterialDef>> const& materials) {
    SceneDef s;
    assert(j.at("version").get<uint32_t>() == SCENE_JSON_VERSION);
    s.name = j.at("name").get<string>();

    // load the camera
    auto jcam = j.at("camera");
    if (jcam.contains("eye"))
        s.camera.eye = jcam.at("eye").get<glm::vec3>();
    if (jcam.contains("lookAt"))
        s.camera.setLookAt(s.camera.eye, jcam.at("lookAt").get<glm::vec3>());
    if (jcam.contains("nearClip"))
        s.camera.nearClip = jcam.at("nearClip").get<float>();
    if (jcam.contains("farClip"))
        s.camera.farClip = jcam.at("farClip").get<float>();

    // load the lights
    for (auto const& light : j.at("lights").get<vector<json>>()) {
        string type = light.at("type").get<string>();
        if (type == "point") {
            auto pos = light.at("pos").get<glm::vec3>();
            auto color = light.at("color").get<glm::vec3>();
            auto falloffStart = light.contains("falloffStart") ? light.at("falloffStart").get<float>() : 0.f;
            auto falloffEnd = light.contains("falloffEnd") ? light.at("falloffEnd").get<float>() : 0.f;
            s.pointLights.push_back(make_shared<VulkPointLight>(pos, falloffStart, color, falloffEnd));
        } else {
            VULK_THROW("Unknown light type: " + type);
        }
    };

    // load the actors
    for (auto const& actor : j.at("actors").get<vector<nlohmann::json>>()) {
        auto a = make_shared<ActorDef>(ActorDef::fromJSON(actor, pipelines, models, meshes, materials));
        s.actors.push_back(a);
        assert(!s.actorMap.contains(a->name));
        s.actorMap[a->name] = a;
    }
    s.validate();
    return s;
}

void findAndProcessMetadata(const fs::path path, Metadata& metadata) {
    cout << "Finding and processing metadata in " << path << endl;
    assert(fs::exists(path) && fs::is_directory(path));

    // The metadata is stored in JSON files with the following extensions
    // other extensions need special handling or no handling (e.g. .mtl files are handled by
    // loadMaterialDef, and .obj and .spv files are handled directly)
    static set<string> jsonExts{".model", ".scene"};
    static set<string> binExts{".pipeline.bin"};
    struct LoadInfo {
        json j;
        fs::path path;
    };
    unordered_map<string, unordered_map<string, LoadInfo>> loadInfos;

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            string stem = entry.path().stem().string();
            string ext = entry.path().stem().extension().string() + entry.path().extension().string(); // get 'bar' from foo.bar and 'bar.bin' from foo.bar.bin
            if (jsonExts.contains(ext)) {
                ifstream f(entry.path());
                LoadInfo loadInfo;
                loadInfo.j = nlohmann::json::parse(f, nullptr, true, true); // allow comments
                loadInfo.path = entry.path().parent_path();
                assert(loadInfo.j.at("name") == stem);
                loadInfos[ext][loadInfo.j.at("name")] = loadInfo;
            } else if (binExts.contains(ext)) {
                if (ext == ".pipeline.bin") {
                    auto pipelineDef = make_shared<BuiltPipelineDef>();
                    VulkCereal::inst()->fromFile(entry.path(), *pipelineDef);
                    assert(!metadata.pipelines.contains(pipelineDef->name));
                    metadata.pipelines[pipelineDef->name] = pipelineDef;
                } else {
                    VULK_THROW("Unknown bin extension: " + ext);
                }
            } else if (ext == ".vertspv") {
                assert(!metadata.vertShaders.contains(stem));
                metadata.vertShaders[stem] = make_shared<vulk::ShaderDef>();
                metadata.vertShaders[stem]->name = stem;
                metadata.vertShaders[stem]->path = entry.path().string();

            } else if (ext == ".geomspv") {
                assert(!metadata.geometryShaders.contains(stem));
                metadata.geometryShaders[stem] = make_shared<vulk::ShaderDef>();
                metadata.geometryShaders[stem]->name = stem;
                metadata.geometryShaders[stem]->path = entry.path().string();

            } else if (ext == ".fragspv") {
                assert(!metadata.fragmentShaders.contains(stem));
                metadata.fragmentShaders[stem] = make_shared<vulk::ShaderDef>();
                metadata.fragmentShaders[stem]->name = stem;
                metadata.fragmentShaders[stem]->path = entry.path().string();

            } else if (ext == ".mtl") {
                assert(!metadata.materials.contains(stem));
                auto material = make_shared<MaterialDef>(loadMaterialDef(entry.path().string()));
                metadata.materials[material->name] = material;
            } else if (ext == ".obj") {
                if (metadata.meshes.contains(stem)) {
                    cerr << "Mesh already exists: " << stem << endl;
                }
                assert(!metadata.meshes.contains(stem));
                ModelMeshDef mmd{entry.path().string()};
                metadata.meshes[stem] = make_shared<MeshDef>(stem, mmd);
            }
        }
    }

    // The order matters here: models depend on meshes and materials, actors depend on models and pipelines
    // and the scene depends on actors

    for (auto const& [name, pipeline] : metadata.pipelines) {
        pipeline->fixup(metadata.vertShaders, metadata.geometryShaders, metadata.fragmentShaders);
    }

    for (auto const& [name, loadInfo] : loadInfos[".model"]) {
        auto modelDef = make_shared<ModelDef>(ModelDef::fromJSON(loadInfo.j, metadata.meshes, metadata.materials));
        assert(!metadata.models.contains(modelDef->name));
        metadata.models[modelDef->name] = modelDef;
    }

    for (auto const& [name, loadInfo] : loadInfos[".scene"]) {
        auto sceneDef = make_shared<SceneDef>(SceneDef::fromJSON(loadInfo.j, metadata.pipelines, metadata.models, metadata.meshes, metadata.materials));
        assert(!metadata.scenes.contains(sceneDef->name));
        metadata.scenes[sceneDef->name] = sceneDef;
    }

    if (metadata.scenes.size() == 0) {
        cerr << "No scenes found in " << path << " something is probably wrong\n";
        VULK_THROW("No scenes found in " + path.string());
    }
}

std::filesystem::path getResourcesDir() {
    static std::filesystem::path path;
    static once_flag flag;
    call_once(flag, [&]() {
        std::filesystem::path config_path = fs::current_path();
        for (int i = 0; i < 5; i++) {
            if (fs::exists(config_path / "config.json")) {
                break;
            }
            config_path = config_path.parent_path();
        }
        cout << "loading config for config_path " << config_path << endl;
        ifstream config_ifstream(config_path / "config.json");
        assert(config_ifstream.is_open() && "Failed to open config.json");
        auto config = nlohmann::json::parse(config_ifstream, nullptr, true, true); // allow comments
        path = config.at("ResourcesDir").get<string>();
    });
    return path;
}

Metadata const* getMetadata() {
    static Metadata metadata;
    static once_flag flag;
    call_once(flag, [&]() {
        fs::path path = getResourcesDir();
        findAndProcessMetadata(path, metadata);
    });

    return &metadata;
}
