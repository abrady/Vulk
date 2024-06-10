#include <filesystem>
#include <iostream>
#include <memory>

#include "CLI/CLI.hpp"

#include <filesystem>

#include "BuildPipeline.h"
#include "Vulk/VulkLogger.h"
#include "Vulk/VulkUtil.h"

void glslShaderEnumsGenerator(fs::path outFile, bool verbose) {
    auto logger = VulkLogger::CreateLogger("Buildtool:ShaderEnumsGenerator");
    logger->info("GLSLIncludesGenerator: Generating GLSL includes for enum values to: {}", outFile.string());
    auto parent_dir = outFile.parent_path();
    if (!fs::exists(parent_dir)) {
        logger->error("Output directory does not exist: {}", parent_dir.string());
        throw CLI::ValidationError("Output directory does not exist: " + parent_dir.string());
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
    for (size_t i = 0; i < apache::thrift::TEnumDataStorage<vulk::cpp2::VulkShaderBinding>::values.size(); ++i) {
        auto value = apache::thrift::TEnumDataStorage<vulk::cpp2::VulkShaderBinding>::names[i];
        auto key = apache::thrift::TEnumDataStorage<vulk::cpp2::VulkShaderBinding>::values[i];
        out << "const int VulkShaderBinding_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkShaderBinding_{} = {};", value, (int)key);
    }

    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    for (size_t i = 0; i < apache::thrift::TEnumDataStorage<vulk::cpp2::VulkShaderLocation>::values.size(); ++i) {
        auto value = apache::thrift::TEnumDataStorage<vulk::cpp2::VulkShaderLocation>::names[i];
        auto key = apache::thrift::TEnumDataStorage<vulk::cpp2::VulkShaderLocation>::values[i];
        out << "const int VulkShaderLocation_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkShaderLocation_{} = {};", value, (int)key);
    }

    // Write the light constants
    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    for (size_t i = 0; i < apache::thrift::TEnumDataStorage<vulk::cpp2::VulkLights>::values.size(); ++i) {
        auto value = apache::thrift::TEnumDataStorage<vulk::cpp2::VulkLights>::names[i];
        auto key = apache::thrift::TEnumDataStorage<vulk::cpp2::VulkLights>::values[i];
        out << "const int VulkLights_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkLights_{} = {};", value, (int)key);
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
    auto logger = VulkLogger::CreateLogger("findSrcMetadata");
    logger->info("Finding src metadata in {}", path.string());
    assert(fs::exists(path) && fs::is_directory(path));

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        string stem = entry.path().stem().string();
        string ext = entry.path().stem().extension().string() + entry.path().extension().string(); // get 'bar' from foo.bar and 'bar.bin' from foo.bar.bin
        if (ext == ".glsl") {
            metadata.shaderIncludes[stem] = entry.path();
        } else if (ext == ".vert") {
            metadata.vertShaders[stem] = entry.path();
        } else if (ext == ".geom") {
            metadata.geometryShaders[stem] = entry.path();
        } else if (ext == ".frag") {
            metadata.fragmentShaders[stem] = entry.path();
        } else if (ext == ".mtl") {
            metadata.materials[stem] = entry.path();
        } else if (ext == ".obj") {
            metadata.meshes[stem] = entry.path();
        } else if (ext == ".model") {
            metadata.models[stem] = entry.path();
        } else if (ext == ".pipeline") {
            metadata.pipelines[stem] = entry.path();
        } else if (ext == ".scene") {
            metadata.scenes[stem] = entry.path();
        }
    }
    VULK_ASSERT_FMT(metadata.scenes.size() > 0, "No scenes found in {}", path.string());
}

// builds the shader in the build directory so it can be loaded by the pipeline
// TODO: compare timestamps and only rebuild if necessary
vulk::cpp2::ShaderDef buildShaderDef(fs::path srcShaderPath, fs::path buildDir, fs::path generatedHeaderDir) {
    VULK_ASSERT(fs::exists(srcShaderPath) && fs::is_regular_file(srcShaderPath));
    VULK_ASSERT(fs::exists(buildDir) && fs::is_directory(buildDir));

    fs::path commonDir = srcShaderPath.parent_path().parent_path() / "Common";
    VULK_ASSERT(fs::exists(commonDir) && fs::is_directory(commonDir));

    std::string shaderDir = srcShaderPath.extension().string().substr(1);
    if (!fs::exists(buildDir / shaderDir)) {
        VULK_ASSERT_FMT(fs::create_directory(buildDir / shaderDir), "Failed to create directory: {}", (buildDir / shaderDir).string());
    }
    fs::path dstShaderPath = buildDir / shaderDir / (srcShaderPath.filename().string() + "spv");
    std::string cmd = "glslc -g --target-env=vulkan1.3";
#ifdef DEBUG
    cmd += " -O0";
#endif
    cmd += " -I" + generatedHeaderDir.string() + " -I" + commonDir.string();
    cmd += " -o " + dstShaderPath.string() + " " + srcShaderPath.string();
    int result = std::system(cmd.c_str());
    VULK_ASSERT_FMT(result == 0, "Failed to compile shader: {}", cmd);

    vulk::cpp2::ShaderDef shaderOut;
    shaderOut.name_ref() = srcShaderPath.stem().string();
    shaderOut.path_ref() = dstShaderPath.lexically_relative(buildDir).string();
    return shaderOut;
}

vulk::cpp2::PipelineDef buildPipelineAndShaders(const SrcMetadata& metadata, vulk::cpp2::SrcPipelineDef srcPipelineDef, fs::path buildDir, fs::path generatedHeaderDir) {
    vulk::cpp2::PipelineDef pipelineDef;
    pipelineDef.name_ref() = srcPipelineDef.get_name();

    // first build the shaders so we can reference them when we build the descriptor sets etc.

    if (srcPipelineDef.get_vertShader() != "") {
        fs::path path = metadata.vertShaders.at(srcPipelineDef.get_vertShader());
        buildShaderDef(path, buildDir, generatedHeaderDir);
    }
    if (srcPipelineDef.get_geomShader() != "") {
        buildShaderDef(metadata.geometryShaders.at(srcPipelineDef.get_geomShader()), buildDir, generatedHeaderDir);
    }
    if (srcPipelineDef.get_fragShader() != "") {
        buildShaderDef(metadata.fragmentShaders.at(srcPipelineDef.get_fragShader()), buildDir, generatedHeaderDir);
    }

    // build the pipeline with the built shaders
    return PipelineBuilder::buildPipeline(srcPipelineDef, buildDir);
}

// This is the main entry point for building a project definition from a project file.
// it searches the assets in the passed in directory and builds only those referenced
// by the project itself.
void buildProjectDef(const fs::path project_file_path, fs::path buildDir) {
    auto logger = VulkLogger::CreateLogger("buildProjectDef");
    fs::path projectDir = project_file_path.parent_path();
    logger->info("Building project from {}", project_file_path.string());
    VULK_ASSERT(fs::exists(project_file_path));
    VULK_ASSERT(fs::exists(projectDir) && fs::is_directory(projectDir));
    VULK_ASSERT(fs::exists(buildDir) && fs::is_directory(buildDir));

    // first load everything in the project directory
    SrcMetadata metadata = {};
    findSrcMetadata(projectDir, metadata);

    // then use the project as the root for what is actually referenced.
    vulk::cpp2::SrcProjectDef projectIn;
    readDefFromFile(project_file_path.string(), projectIn);

    // build the shader includes
    fs::path commonShaderHeadersDir = buildDir / "common";
    if (!fs::exists(commonShaderHeadersDir)) {
        VULK_ASSERT(fs::create_directory(commonShaderHeadersDir));
    }
    glslShaderEnumsGenerator(commonShaderHeadersDir / "VulkShaderEnums_generated.glsl", false);

    for (auto& [shaderName, shaderPath] : metadata.shaderIncludes) {
        fs::copy_file(shaderPath, commonShaderHeadersDir / shaderPath.filename(), fs::copy_options::overwrite_existing);
    }

    // build the shaders, pipelines and models
    vulk::cpp2::ProjectDef projectOut;
    for (string sceneName : projectIn.get_sceneNames()) {
        readDefFromFile(metadata.scenes.at(sceneName).string(), projectOut.scenes_ref()[sceneName]);
        auto& scene = projectOut.scenes_ref()[sceneName];
        for (auto& actorDef : scene.actors_ref().value()) {
            std::string pipelineName = actorDef.get_pipeline();
            if (!projectOut.get_pipelines().contains(pipelineName)) {
                vulk::cpp2::SrcPipelineDef srcPipelineDef;
                readDefFromFile(metadata.pipelines.at(pipelineName).string(), srcPipelineDef);
                projectOut.pipelines_ref()[pipelineName] = buildPipelineAndShaders(metadata, srcPipelineDef, buildDir, commonShaderHeadersDir);
            }

            std::string modelName = actorDef.get_modelName();
            if (modelName != "" && !projectOut.get_models().contains(modelName)) {
                readDefFromFile((projectDir / actorDef.get_modelName()).string(), projectOut.models_ref()[modelName]);
            }
        }
    }

    if (projectOut.get_scenes().size() == 0) {
        logger->error("No scenes found in {}", project_file_path.string());
        VULK_THROW("No scenes found in {}", project_file_path.string());
    }

    writeDefToFile((buildDir / project_file_path.filename()).string(), projectOut);
}
