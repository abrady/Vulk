#define _WIN32_WINNT 0x0A00  // for boost
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

#include "BuildPipeline.h"
#include "Vulk/VulkLogger.h"
#include "Vulk/VulkUtil.h"

static std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("BuildProject");

namespace at  = apache::thrift;
namespace bp  = boost::process;
namespace vk2 = vulk::cpp2;

static int runProcess(const string cmd, std::string& out) {
    // Stream to capture output and error
    boost::process::ipstream combinedStream;
    boost::process::child c(cmd, boost::process::std_out > combinedStream, boost::process::std_err > combinedStream);
    // Read the output
    std::string line;
    while (c.running() && std::getline(combinedStream, line) && !line.empty()) {
        out += line + "\n";
    }
    // Wait for the process to finish and collect the exit code
    c.wait();
    // Return the exit code of the process
    return c.exit_code();
}

static void makeDir(fs::path dir) {
    if (!fs::exists(dir)) {
        VULK_ASSERT(fs::create_directories(dir));
    }
    VULK_ASSERT(fs::is_directory(dir));
}

void glslShaderEnumsGenerator(fs::path outFile, bool verbose) {
    logger->trace("GLSLIncludesGenerator: Generating GLSL includes for enum values to: {}", outFile.string());
    auto parent_dir = outFile.parent_path();
    if (!fs::exists(parent_dir)) {
        logger->error("Output directory does not exist: {}", parent_dir.string());
        VULK_THROW("Output directory does not exist: {}", parent_dir.string());
    }

    if (verbose) {
        logger->set_level(spdlog::level::trace);
    }
    // logger->set_level(spdlog::level::trace);

    std::ofstream out(outFile);
    out << R"(
// Generated header file for enum values coming from our headers
// e.g. UBO bindings, or layout locations
)";

    // Write the UBO bindings
    out << "\n// UBO Bindings\n";
    logger->trace("// UBO Bindings");
    for (size_t i = 0; i < at::TEnumDataStorage<vk2::VulkShaderBinding>::values.size(); ++i) {
        auto value = at::TEnumDataStorage<vk2::VulkShaderBinding>::names[i];
        auto key   = at::TEnumDataStorage<vk2::VulkShaderBinding>::values[i];
        out << "const int Binding_" << value << " = " << (int)key << ";\n";
        logger->trace("const int Binding_{} = {};", value, (int)key);
    }

    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    for (size_t i = 0; i < at::TEnumDataStorage<vk2::VulkShaderLocation>::values.size(); ++i) {
        auto value = at::TEnumDataStorage<vk2::VulkShaderLocation>::names[i];
        auto key   = at::TEnumDataStorage<vk2::VulkShaderLocation>::values[i];
        out << "const int VulkShaderLocation_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkShaderLocation_{} = {};", value, (int)key);
    }

    // Write the light constants
    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    for (size_t i = 0; i < at::TEnumDataStorage<vk2::VulkLights>::values.size(); ++i) {
        auto value = at::TEnumDataStorage<vk2::VulkLights>::names[i];
        auto key   = at::TEnumDataStorage<vk2::VulkLights>::values[i];
        out << "const int VulkLights_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkLights_{} = {};", value, (int)key);
    }

    // Write the gbuf Inputs (out locations)
    out << "\n// GBuffer Inputs\n";
    for (size_t i = 0; i < at::TEnumDataStorage<vk2::GBufInputAtmtIdx>::values.size(); ++i) {
        auto value = at::TEnumDataStorage<vk2::GBufInputAtmtIdx>::names[i];
        auto key   = at::TEnumDataStorage<vk2::GBufInputAtmtIdx>::values[i];
        out << "const int GBufInputAtmtIdx_" << value << " = " << (int)key << ";\n";
        logger->trace("const int GBufInputAtmtIdx_{} = {};", value, (int)key);
    }

    out << "\n";

    out.close();

    // // throw CLI::Success(); // not really necessary
}

struct SrcMetadata {
    unordered_map<string, fs::path> meshes;
    unordered_map<string, fs::path> shaderIncludes;
    unordered_map<string, fs::path> vertShaders;
    unordered_map<string, fs::path> geometryShaders;
    unordered_map<string, fs::path> fragmentShaders;
    unordered_map<string, fs::path> materials;
    unordered_map<string, fs::path> models;
    unordered_map<string, fs::path> pipelines;
    unordered_map<string, fs::path> scenes;
};

void findSrcMetadata(const fs::path path, SrcMetadata& metadata) {
    logger->trace("Finding src metadata in {}", path.string());
    assert(fs::exists(path) && fs::is_directory(path));

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        string stem = entry.path().stem().string();
        string ext  = entry.path().stem().extension().string() +
                     entry.path().extension().string();  // get 'bar' from foo.bar and 'bar.bin' from foo.bar.bin
        if (ext == ".glsl") {
            VULK_ASSERT(!metadata.shaderIncludes.contains(stem), "Duplicate shader include found: {}", stem);
            metadata.shaderIncludes[stem] = entry.path();
        } else if (ext == ".vert") {
            VULK_ASSERT(!metadata.vertShaders.contains(stem), "Duplicate vertex shader found: {}", stem);
            metadata.vertShaders[stem] = entry.path();
        } else if (ext == ".geom") {
            VULK_ASSERT(!metadata.geometryShaders.contains(stem), "Duplicate geometry shader found: {}", stem);
            metadata.geometryShaders[stem] = entry.path();
        } else if (ext == ".frag") {
            VULK_ASSERT(!metadata.fragmentShaders.contains(stem), "Duplicate fragment shader found: {}", stem);
            metadata.fragmentShaders[stem] = entry.path();
        } else if (ext == ".mtl") {
            VULK_ASSERT(!metadata.materials.contains(stem), "Duplicate material found: {}", stem);
            metadata.materials[stem] = entry.path();
        } else if (ext == ".obj") {
            VULK_ASSERT(!metadata.meshes.contains(stem), "Duplicate model found: {}", stem);
            metadata.meshes[stem] = entry.path();
        } else if (ext == ".model") {
            VULK_ASSERT(!metadata.models.contains(stem), "Duplicate model found: {}", stem);
            metadata.models[stem] = entry.path();
        } else if (ext == ".pipeline") {
            VULK_ASSERT(!metadata.pipelines.contains(stem), "Duplicate pipeline found: {}", stem);
            metadata.pipelines[stem] = entry.path();
        } else if (ext == ".scene") {
            VULK_ASSERT(!metadata.scenes.contains(stem), "Duplicate scene found: {}", stem);
            metadata.scenes[stem] = entry.path();
        }
    }
    VULK_ASSERT(metadata.scenes.size() > 0, "No scenes found in {}", path.string());
}

static bool shouldCopyFile(fs::path src, fs::path dst) {
    // Check if both files exist
    VULK_ASSERT(fs::exists(src) && fs::is_regular_file(src), "src File {} does not exist", src.string());
    if (!fs::exists(dst)) {
        logger->trace("dst File {} does not exist", dst.string());
        return true;
    }
    VULK_ASSERT(fs::is_regular_file(dst), "dst File {} is not a regular file", dst.string());
    // Get the last modification time of the source and destination files
    auto srcTime = fs::last_write_time(src);
    auto dstTime = fs::last_write_time(dst);
    // Return true if the source file is newer than the destination file
    return srcTime > dstTime;
}

// copies a file if it should be copied
// also creates the parent directory if it doesn't exist
static void copyFileIfShould(fs::path src, fs::path dst) {
    if (shouldCopyFile(src, dst)) {
        logger->info("Copying file: {} to {}", src.string(), dst.string());
        VULK_ASSERT(fs::exists(dst.parent_path()) || fs::create_directories(dst.parent_path()));
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
    } else {
        logger->trace("Skipping file copy: {} to {}", src.string(), dst.string());
    }
}

static void copyDirIfShould(fs::path src, fs::path dst) {
    // Check if source directory exists
    assert(fs::exists(src) && fs::is_directory(src));

    // Create destination directory if it does not exist
    if (!fs::exists(dst)) {
        fs::create_directories(dst);
    }
    // Iterate through the source directory
    std::unordered_set<std::string> sourceFiles;
    for (const auto& entry : fs::directory_iterator(src)) {
        // Assert if a subdirectory is found, just handle files in leaf directories for now
        VULK_ASSERT(!fs::is_directory(entry.path()));
        std::string filename = entry.path().filename().string();
        sourceFiles.insert(filename);
        fs::path dest_path = dst / filename;
        copyFileIfShould(entry.path(), dest_path);
    }
    // Check for files in dst that are not in src
    for (const auto& entry : fs::directory_iterator(dst)) {
        VULK_ASSERT(!fs::is_directory(entry.path()));
        std::string filename = entry.path().filename().string();
        VULK_ASSERT(sourceFiles.contains(filename), "File in destination not in source: {}", filename);
    }
}

// builds the shader in the build directory so it can be loaded by the pipeline
// TODO: compare timestamps and only rebuild if necessary
static vk2::ShaderDef buildShaderDef(fs::path srcShaderPath, fs::path buildDir, fs::path generatedHeaderDir) {
    VULK_ASSERT(fs::exists(srcShaderPath) && fs::is_regular_file(srcShaderPath));

    fs::path commonDir = srcShaderPath.parent_path().parent_path() / "Common";
    makeDir(commonDir);

    VULK_ASSERT(fs::exists(commonDir) && fs::is_directory(commonDir));

    std::string shaderDir = srcShaderPath.extension().string().substr(1);
    if (!fs::exists(buildDir / shaderDir)) {
        makeDir(buildDir / shaderDir);
    }
    fs::path dstShaderPath = buildDir / shaderDir / (srcShaderPath.filename().string() + "spv");

    if (shouldCopyFile(srcShaderPath, dstShaderPath)) {
        logger->info("Building shader: {}", srcShaderPath.string());
        std::string cmd = "glslc -g --target-env=vulkan1.3";
#ifdef DEBUG
        cmd += " -O0";
#endif
        cmd += " -I" + generatedHeaderDir.string() + " -I" + commonDir.string();
        cmd += " -o " + dstShaderPath.string() + " " + srcShaderPath.string();
        std::string out;
        int result = runProcess(cmd, out);
        VULK_ASSERT(result == 0, "Failed to compile shader: {}, output:\n{}", cmd, out);
    } else {
        logger->trace("Skipping shader already built: {}", srcShaderPath.string());
    }

    vk2::ShaderDef shaderOut;
    shaderOut.name_ref() = srcShaderPath.stem().string();
    shaderOut.path_ref() = dstShaderPath.lexically_relative(buildDir).string();
    return shaderOut;
}

void buildPipelineAndShaders(const SrcMetadata& metadata,
                             vk2::SrcPipelineDef srcPipelineDef,
                             fs::path shadersBuildDir,
                             fs::path generatedHeaderDir) {
    vk2::PipelineDef pipelineDef;
    pipelineDef.name_ref() = srcPipelineDef.get_name();

    // first build the shaders so we can reference them when we build the descriptor sets etc.
    if (srcPipelineDef.get_vertShader() != "") {
        VULK_ASSERT(metadata.vertShaders.contains(srcPipelineDef.get_vertShader()),
                    "Vertex shader {} not found",
                    srcPipelineDef.get_vertShader());
        fs::path path = metadata.vertShaders.at(srcPipelineDef.get_vertShader());
        buildShaderDef(path, shadersBuildDir, generatedHeaderDir);
    }
    if (srcPipelineDef.get_geomShader() != "") {
        VULK_ASSERT(metadata.geometryShaders.contains(srcPipelineDef.get_geomShader()),
                    "Geometry shader {} not found",
                    srcPipelineDef.get_geomShader());
        buildShaderDef(metadata.geometryShaders.at(srcPipelineDef.get_geomShader()), shadersBuildDir, generatedHeaderDir);
    }
    if (srcPipelineDef.get_fragShader() != "") {
        VULK_ASSERT(metadata.fragmentShaders.contains(srcPipelineDef.get_fragShader()),
                    "Fragment shader {} not found",
                    srcPipelineDef.get_fragShader());
        buildShaderDef(metadata.fragmentShaders.at(srcPipelineDef.get_fragShader()), shadersBuildDir, generatedHeaderDir);
    }

    // build the pipeline with the built shaders
    PipelineBuilder::buildPipelineFile(srcPipelineDef,
                                       shadersBuildDir,
                                       shadersBuildDir.parent_path() / "Pipelines" / (srcPipelineDef.get_name() + ".pipeline"));
}

// This is the main entry point for building a project definition from a project file.
// it searches the assets in the passed in directory and builds only those referenced
// by the project itself.
void buildProjectDef(const fs::path project_file_path, fs::path buildDir) {
    fs::path projectDir = project_file_path.parent_path();
    logger->trace("Building project from {}", project_file_path.string());
    VULK_ASSERT(fs::exists(project_file_path), "Project file does not exist: {}", project_file_path.string());
    VULK_ASSERT(fs::exists(projectDir) && fs::is_directory(projectDir),
                "Project directory does not exist: {}",
                projectDir.string());

    // due to the complexities of not being able to get the build diredctory until
    // generation time in cmake we can't get the build directory 'generation' time
    // which means we can't mkdir it during configuration time. So we have to do it here.
    fs::path assetsDir = buildDir / "Assets";
    makeDir(assetsDir);

    // first load everything in the project directory
    SrcMetadata metadata = {};
    findSrcMetadata(projectDir, metadata);

    // then use the project as the root for what is actually referenced.
    vk2::SrcProjectDef projectIn;
    readDefFromFile(project_file_path.string(), projectIn);

    // build the shader includes
    fs::path commonShaderHeadersDir = assetsDir / "common";
    if (!fs::exists(commonShaderHeadersDir)) {
        VULK_ASSERT(fs::create_directory(commonShaderHeadersDir));
    }
    glslShaderEnumsGenerator(commonShaderHeadersDir / "VulkShaderEnums_generated.glsl", false);

    for (auto& [shaderName, shaderPath] : metadata.shaderIncludes) {
        fs::copy_file(shaderPath, commonShaderHeadersDir / shaderPath.filename(), fs::copy_options::overwrite_existing);
    }

    // build the shaders, pipelines and models
    vk2::ProjectDef projectOut;
    for (string sceneName : projectIn.get_sceneNames()) {
        if (!metadata.scenes.contains(sceneName)) {
            logger->error("Scene {} in {} doesn't exist. missing scene file?", sceneName, project_file_path.string());
            VULK_THROW("Scene {} in {} doesn't exist. missing scene file?", sceneName, project_file_path.string());
        }
        fs::path scenePath = metadata.scenes.at(sceneName);
        copyFileIfShould(scenePath, assetsDir / "Scenes" / scenePath.filename());
        readDefFromFile(scenePath.string(), projectOut.scenes_ref()[sceneName]);
        auto& scene = projectOut.scenes_ref()[sceneName];
        for (auto& actorDef : scene.actors_ref().value()) {
            std::string modelName   = actorDef.get_modelName();
            vk2::ModelDef* modelDef = nullptr;
            if (modelName != "") {
                if (!projectOut.get_models().contains(modelName)) {
                    fs::path modelPath = (projectDir / (actorDef.get_modelName() + ".model"));
                    VULK_ASSERT(metadata.models.contains(modelName), "Model {} not found", modelName);
                    copyFileIfShould(metadata.models.at(modelName),
                                     assetsDir / "Models" / metadata.models.at(modelName).filename());
                    modelDef = &projectOut.models_ref()[modelName];
                    readDefFromFile(modelPath.string(), *modelDef);
                }
            } else {
                // inline model
                modelDef = &actorDef.inlineModel_ref().value();
            }
            if (modelDef) {
                // copy materials
                VULK_ASSERT(metadata.materials.contains(modelDef->get_material()),
                            "Material {} not found",
                            modelDef->get_material());
                fs::path materialPath = metadata.materials.at(modelDef->get_material());
                copyDirIfShould(materialPath.parent_path(), assetsDir / "Materials" / materialPath.parent_path().filename());
            }
        }
    }

    // build all the pipelines, some aren't referenced by the project so we need to build them all
    for (auto [pipelineName, pipelinePath] : metadata.pipelines) {
        if (!projectOut.get_pipelines().contains(pipelineName)) {
            vk2::SrcPipelineDef srcPipelineDef;
            copyFileIfShould(pipelinePath, assetsDir / "Pipelines" / pipelinePath.filename());
            readDefFromFile(pipelinePath.string(), srcPipelineDef);
            buildPipelineAndShaders(metadata, srcPipelineDef, assetsDir / "Shaders", commonShaderHeadersDir);
        }
    }

    if (projectOut.get_scenes().size() == 0) {
        logger->error("No scenes found in {}", project_file_path.string());
        VULK_THROW("No scenes found in {}", project_file_path.string());
    }

    projectOut.name_ref()          = projectIn.get_name();
    projectOut.startingScene_ref() = projectIn.get_startingScene();
    writeDefToFile((buildDir / project_file_path.filename()).string(), projectOut);
}
