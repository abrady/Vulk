#include "Vulk/VulkResourceMetadata.h"
#include <cstdlib>

using namespace std;

namespace fs = std::filesystem;

static std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("VulkResourceMetadata");

// since we're loading data from files and because thrift only supports
// double just convert quietly in this file only so we can load the data
// without warnings
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif

#pragma warning(disable : 4244)  // double to float

MaterialDef loadMaterialDef(const fs::path& file) {
    if (!fs::exists(file)) {
        VULK_THROW("Material file does not exist: {}", file.string());
    }

    std::ifstream mtlFile(file);
    if (!mtlFile.is_open()) {
        VULK_THROW("Failed to open material file: {}", file.string());
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
                VULK_THROW("Referenced file does not exist: {}", absPath.string());
            }
            return absPath.string();
        };

        if (prefix == "#" || prefix == "") {
            continue;
        } else if (prefix == "newmtl") {
            assert(!startedNewMtl);
            startedNewMtl = true;  // just handle 1 material for now
            string mtlName;
            lineStream >> mtlName;
            assert(mtlName == file.stem().string());
            material.name = mtlName;
        } else if (prefix == "map_Ka") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKa = processPath(relativePath);  // ambient texture
        } else if (prefix == "map_Kd") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKd = processPath(relativePath);  // diffuse texture
        } else if (prefix == "map_Ks") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapKs = processPath(relativePath);  // specular texture
        } else if (prefix == "map_Bump" || prefix == "norm") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapNormal = processPath(relativePath);  // normal map texture
        } else if (prefix == "map_Pm") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapPm = processPath(relativePath);  // metalness map texture
        } else if (prefix == "map_Pr") {
            std::string relativePath;
            lineStream >> relativePath;
            material.mapPr = processPath(relativePath);  // roughness map texture
        } else if (prefix == "disp") {
            std::string relativePath;
            lineStream >> relativePath;
            material.disp = processPath(relativePath);  // displacement map texture
        } else if (prefix == "cubemap") {
            std::string relativePath;
            for (int i = 0; i < 6; i++) {
                lineStream >> relativePath;
                material.cubemapImgs[i] = processPath(relativePath);  // cubemap texture
            }
        } else if (prefix == "Ns") {
            lineStream >> material.Ns;  // specular exponent
        } else if (prefix == "Ni") {
            lineStream >> material.Ni;  // index of refraction
        } else if (prefix == "d" || prefix == "Tr") {
            lineStream >> material.d;  // dissolve (transparency)
        } else if (prefix == "Ka") {
            lineStream >> material.Ka[0] >> material.Ka[1] >> material.Ka[2];  // ambient color
        } else if (prefix == "Kd") {
            lineStream >> material.Kd[0] >> material.Kd[1] >> material.Kd[2];  // diffuse color
        } else if (prefix == "Ks") {
            lineStream >> material.Ks[0] >> material.Ks[1] >> material.Ks[2];  // specular color
        } else {
            VULK_THROW("Unknown material property: {}", prefix);
        }
        // Add handling for other properties as needed
    }

    return material;
}

ModelDef ModelDef::fromDef(vulk::cpp2::ModelDef const& defIn,
                           unordered_map<string, shared_ptr<MeshDef>> const& meshes,
                           unordered_map<string, shared_ptr<MaterialDef>> materials) {
    auto name = defIn.name().value();
    string mn = defIn.get_material();
    auto material = materials.at(mn);
    vulk::cpp2::MeshDefType meshDefType = defIn.meshDefType().value();
    switch (meshDefType) {
        case vulk::cpp2::MeshDefType::Model: {
            std::string meshName = defIn.get_mesh();
            return ModelDef(name, meshes.at(meshName), material);
        }
        case vulk::cpp2::MeshDefType::Mesh: {
            shared_ptr<VulkMesh> mesh = make_shared<VulkMesh>();
            vulk::cpp2::GeoMeshDef def = defIn.geoMesh().value();

            auto geoMeshType = defIn.get_geoMesh().getType();
            switch (geoMeshType) {
                case vulk::cpp2::GeoMeshDef::Type::sphere: {
                    vulk::cpp2::GeoSphereDef const& sphere = def.get_sphere();
                    makeGeoSphere(sphere.get_radius(), sphere.get_numSubdivisions(), *mesh);
                } break;
                case vulk::cpp2::GeoMeshDef::Type::cylinder: {
                    vulk::cpp2::GeoCylinderDef const& cylinder = def.get_cylinder();
                    makeCylinder(cylinder.get_height(), cylinder.get_bottomRadius(), cylinder.get_topRadius(),
                                 cylinder.get_numStacks(), cylinder.get_numSlices(), *mesh);
                } break;
                case vulk::cpp2::GeoMeshDef::Type::triangle: {
                    makeEquilateralTri(def.get_triangle().get_sideLength(), def.get_triangle().get_numSubdivisions(),
                                       *mesh);
                } break;
                case vulk::cpp2::GeoMeshDef::Type::quad: {
                    makeQuad(def.get_quad().get_w(), def.get_quad().get_h(), def.get_quad().get_numSubdivisions(),
                             *mesh);
                } break;
                case vulk::cpp2::GeoMeshDef::Type::grid: {
                    makeGrid(def.get_grid().get_width(), def.get_grid().get_depth(), def.get_grid().get_m(),
                             def.get_grid().get_n(), *mesh, def.get_grid().get_repeatU(), def.get_grid().get_repeatV());
                } break;
                case vulk::cpp2::GeoMeshDef::Type::axes: {
                    makeAxes(def.get_axes().get_length(), *mesh);
                } break;
                default: {
                    VULK_THROW("Unhandled/known GeoMesh type: {}", (int)geoMeshType);
                }
            }
            return ModelDef(name, make_shared<MeshDef>(name, mesh), material);
        }
        default:
            VULK_THROW("Unknown MeshDef type: {}", (int)meshDefType);
    };
}

glm::vec3 toVec3(vulk::cpp2::Vec3 const& v) {
    return glm::vec3(v.get_x(), v.get_y(), v.get_z());
}

ActorDef ActorDef::fromDef(vulk::cpp2::ActorDef defIn,
                           unordered_map<string, shared_ptr<PipelineDef>> const& pipelines,
                           unordered_map<string, shared_ptr<ModelDef>> const& models,
                           unordered_map<string, shared_ptr<MeshDef>> meshes,
                           unordered_map<string, shared_ptr<MaterialDef>> materials) {
    ActorDef a;
    a.def = defIn;
    a.pipeline = pipelines.at(defIn.get_pipeline());

    if (defIn.get_modelName() != "") {
        assert(!defIn.inlineModel().is_set());
        a.model = models.at(defIn.get_modelName());
    } else if (defIn.inlineModel().is_set()) {
        a.model = make_shared<ModelDef>(ModelDef::fromDef(defIn.get_inlineModel(), meshes, materials));
    } else {
        VULK_THROW("ActorDef must contain either a model or an inlineModel");
    }

    // make the transform
    a.xform = glm::mat4(1.0f);
    if (defIn.xform().is_set()) {
        auto& xform = defIn.get_xform();
        glm::vec3 pos = xform.pos().is_set() ? toVec3(xform.get_pos()) : glm::vec3(0);
        glm::vec3 rot = glm::radians(xform.rot().is_set() ? toVec3(xform.get_rot()) : glm::vec3(0));
        glm::vec3 scale = xform.scale().is_set() ? toVec3(xform.get_scale()) : glm::vec3(1);
        a.xform = glm::translate(glm::mat4(1.0f), pos) * glm::eulerAngleXYZ(rot.x, rot.y, rot.z) *
                  glm::scale(glm::mat4(1.0f), scale);
    }
    a.validate();
    return a;
}

SceneDef SceneDef::fromDef(vulk::cpp2::SceneDef defIn,
                           unordered_map<string, shared_ptr<PipelineDef>> const& pipelines,
                           unordered_map<string, shared_ptr<ModelDef>> const& models,
                           unordered_map<string, shared_ptr<MeshDef>> const& meshes,
                           unordered_map<string, shared_ptr<MaterialDef>> const& materials) {
    SceneDef s;
    s.def = defIn;

    // load the camera
    auto& cam = defIn.get_camera();
    s.camera.eye = toVec3(cam.get_eye());
    s.camera.setLookAt(s.camera.eye, toVec3(cam.get_lookAt()));
    s.camera.nearClip = cam.get_nearClip();
    s.camera.farClip = cam.get_farClip();

    // load the lights
    for (auto const& light : defIn.get_lights()) {
        auto type = light.get_type();
        switch (type) {
            case vulk::cpp2::LightType::Point: {
                auto pos = toVec3(light.get_pos());
                auto color = toVec3(light.get_color());
                auto falloffStart = light.falloffStart().is_set() ? light.get_falloffStart() : 0.f;
                auto falloffEnd = light.falloffEnd().is_set() ? light.get_falloffEnd() : 0.f;
                s.pointLights.push_back(make_shared<VulkPointLight>(pos, falloffStart, color, falloffEnd));
            } break;
            default: {
                VULK_THROW("Unknown light type: {}", (int)type);
            }
        }
    }

    // load the actors
    for (auto const& actor : defIn.get_actors()) {
        auto a = make_shared<ActorDef>(ActorDef::fromDef(actor, pipelines, models, meshes, materials));
        s.actors.push_back(a);
        assert(!s.actorMap.contains(a->def.get_name()));
        s.actorMap[a->def.get_name()] = a;
    }
    s.validate();
    return s;
}

void findAndProcessMetadata(const fs::path path, Metadata& metadata) {
    logger->info("Finding and processing metadata in {}", std::filesystem::absolute(path).string());
    assert(fs::exists(path) && fs::is_directory(path));
    metadata.assetsDir = path;

    // The metadata is stored in JSON files with the following extensions
    // other extensions need special handling or no handling (e.g. .mtl files are handled by
    // loadMaterialDef, and .obj and .spv files are handled directly)
    static set<string> fixupExts{".model", ".scene", ".pipeline"};
    struct LoadInfo {
        string filePath;
    };
    unordered_map<string, unordered_map<string, LoadInfo>> loadInfos;

    // for non leaf resources:
    // we gather everything up first because we load and fixup defs at the same time
    // vs. loading all the defs and then doing a fixup pass so each resource
    // is complete when it is loaded.
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        string stem = entry.path().stem().string();
        string ext = entry.path().stem().extension().string() +
                     entry.path().extension().string();  // get 'bar' from foo.bar and 'bar.bin' from foo.bar.bin
        if (fixupExts.contains(ext)) {
            LoadInfo loadInfo;
            loadInfo.filePath = entry.path().string();
            loadInfos[ext][loadInfo.filePath] = loadInfo;
        } else if (ext == ".vertspv") {
            assert(!metadata.vertShaders.contains(stem));
            metadata.vertShaders[stem] = make_shared<vulk::cpp2::ShaderDef>();
            metadata.vertShaders[stem]->name_ref() = stem;
            metadata.vertShaders[stem]->path_ref() = entry.path().string();
        } else if (ext == ".geomspv") {
            assert(!metadata.geometryShaders.contains(stem));
            metadata.geometryShaders[stem] = make_shared<vulk::cpp2::ShaderDef>();
            metadata.geometryShaders[stem]->name_ref() = stem;
            metadata.geometryShaders[stem]->path_ref() = entry.path().string();

        } else if (ext == ".fragspv") {
            assert(!metadata.fragmentShaders.contains(stem));
            metadata.fragmentShaders[stem] = make_shared<vulk::cpp2::ShaderDef>();
            metadata.fragmentShaders[stem]->name_ref() = stem;
            metadata.fragmentShaders[stem]->path_ref() = entry.path().string();
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

    // The order matters here: models depend on meshes and materials, actors depend on models and pipelines
    // and the scene depends on actors

    for (auto const& [name, loadInfo] : loadInfos[".pipeline"]) {
        auto pipeline = make_shared<PipelineDef>();
        readDefFromFile(loadInfo.filePath, pipeline->def);
        metadata.pipelines[pipeline->def.get_name()] = pipeline;
        pipeline->fixup(metadata.vertShaders, metadata.geometryShaders, metadata.fragmentShaders);
    }

    for (auto const& [name, loadInfo] : loadInfos[".model"]) {
        vulk::cpp2::ModelDef def;
        readDefFromFile(loadInfo.filePath, def);
        auto modelDef = make_shared<ModelDef>(ModelDef::fromDef(def, metadata.meshes, metadata.materials));
        assert(!metadata.models.contains(modelDef->name));
        metadata.models[modelDef->name] = modelDef;
    }

    for (auto const& [name, loadInfo] : loadInfos[".scene"]) {
        vulk::cpp2::SceneDef def;
        readDefFromFile(loadInfo.filePath, def);
        auto sceneDef = make_shared<SceneDef>(
            SceneDef::fromDef(def, metadata.pipelines, metadata.models, metadata.meshes, metadata.materials));
        assert(!metadata.scenes.contains(sceneDef->def.get_name()));
        metadata.scenes[sceneDef->def.get_name()] = sceneDef;
    }

    if (metadata.scenes.size() == 0) {
        cerr << "No scenes found in " << path << " something is probably wrong\n";
        VULK_THROW("No scenes found in {}", path.string());
    }
}

std::filesystem::path getResourcesDir() {
    static vulk::cpp2::ResourceConfig config;
    static once_flag flag;
    call_once(flag, [&]() {
        // as part of our cmake build process we write this out under the build directory.
        std::filesystem::path config_path = fs::current_path();
        for (int i = 0; i < 6; i++) {
            if (fs::exists(config_path / "config.json")) {
                break;
            }
            config_path = config_path.parent_path();
        }
        cout << "loading config for config_path " << config_path << endl;
        readDefFromFile((config_path / "config.json").string(), config);
    });
    return config.get_ResourcesDir();
}

std::shared_ptr<const Metadata> getMetadata() {
    static std::shared_ptr<Metadata> metadata;
    static once_flag flag;
    call_once(flag, [&]() {
        fs::path path = getResourcesDir();
        metadata = make_shared<Metadata>();
        findAndProcessMetadata(path, *metadata);
    });

    return metadata;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
