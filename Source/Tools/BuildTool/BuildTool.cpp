#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

#include "CLI/CLI.hpp"

#include <filesystem>

#include "BuildPipeline.h"
#include "Vulk/VulkLogger.h"
#include "Vulk/VulkUtil.h"
#include "VulkShaderEnums_generated.h"

namespace fs = std::filesystem;

int sceneBuilder(fs::path sceneFileIn, fs::path sceneOutDir) {
    std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("buildtool:SceneBuilder");
    logger->info("Building scene from file: {}", sceneFileIn.string());
    if (!fs::exists(sceneFileIn)) {
        logger->error("Scene file does not exist: {}", sceneFileIn.string());
        return 1;
    }
    if (!fs::exists(sceneOutDir)) {
        logger->error("Scene output directory does not exist: {}", sceneOutDir.string());
        return 1;
    }
    // Read from the input file
    std::ifstream sceneIn(sceneFileIn);
    cereal::JSONInputArchive input(sceneIn);
    // SceneDef data;
    // ActorDef data;
    ModelDef data("foo", nullptr, nullptr);
    // ModelMeshDef data2;
    // MeshDef data("foo", data2);
    // MaterialDef data;
    // MyData data;
    input(data); // Deserialize data from the file
    sceneIn.close();
    // Write to the output file
    std::ofstream sceneFileOut(sceneOutDir / sceneFileIn.filename());
    cereal::JSONOutputArchive output(sceneFileOut);
    output(data); // Serialize data to the file
    sceneFileOut.close();
    return 0;
}

int glslShaderEnumsGenerator(fs::path outFile) {
    auto logger = VulkLogger::CreateLogger("Buildtool:ShaderEnumsGenerator");
    logger->info("GLSLIncludesGenerator: Generating GLSL includes for enum values to: {}", outFile.string());
    auto parent_dir = outFile.parent_path();
    if (!fs::exists(parent_dir)) {
        logger->error("Output directory does not exist: {}", parent_dir.string());
        return 1;
    }

    std::ofstream out(outFile);
    out << R"(
// Generated header file for enum values coming from our headers
// e.g. UBO bindings, or layout locations
)";

    // Write the UBO bindings
    out << "\n// UBO Bindings\n";
    logger->trace("// UBO Bindings");

    const VulkShaderBinding *bindings = EnumValuesVulkShaderBinding();
    const char *const *ns = EnumNamesVulkShaderBinding();
    for (int i = 0; ns[i] != nullptr; i++) {
        out << "const int VulkShaderBinding_" << ns[i] << " = " << (int)bindings[i] << ";\n";
        logger->trace("const int VulkShaderBinding_{} = {};", ns[i], (int)bindings[i]);
    }

    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    const VulkShaderLocation *locs = EnumValuesVulkShaderLocation();
    ns = EnumNamesVulkShaderLocation();
    for (int i = 0; ns[i] != nullptr; i++) {
        out << "const int VulkShaderLocation_" << ns[i] << " = " << (int)locs[i] << ";\n";
        logger->trace("const int VulkShaderLocation_{} = {};", ns[i], (int)locs[i]);
    }

    // Write the light constants
    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    auto *lights = EnumValuesVulkLights();
    ns = EnumNamesVulkLights();
    for (int i = 0; ns[i] != nullptr; i++) {
        out << "const int VulkLights_" << ns[i] << " = " << (int)lights[i] << ";\n";
        logger->trace("const int VulkLights_{} = {};", ns[i], (int)lights[i]);
    }

    out << "\n";

    out.close();

    return 0;
}

int pipelineBuilder(fs::path builtShadersDir, fs::path pipelineFileOut, fs::path pipelineFileIn, bool verbose) {
    auto logger = VulkLogger::CreateLogger("PipelineBuilder");
    if (verbose) {
        logger->set_level(spdlog::level::trace);
    }

    logger->info("PipelineBuilder: Building pipeline from file: {}", pipelineFileIn.string());
    if (!fs::exists(builtShadersDir)) {
        logger->error("Shaders directory does not exist: {}", builtShadersDir.string());
        return 1;
    }
    if (!fs::exists(pipelineFileOut.parent_path())) {
        logger->error("Pipeline output directory does not exist: {}", pipelineFileOut.parent_path().string());
        return 1;
    }
    if (!fs::exists(pipelineFileIn)) {
        logger->error("Pipeline file does not exist: {}", pipelineFileIn.string());
        return 1;
    }

    logger->trace("Shaders Dir: {}, Pipeline Out Dir: {}, Processing pipeline: {}", builtShadersDir.string(), pipelineFileOut.string(),
                  pipelineFileIn.string());

    try {
        PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineFileOut, pipelineFileIn);
    } catch (std::exception &e) {
        logger->error("PipelineBuilder: Error: {}", e.what());
        return 1;
    }

    logger->trace("PipelineBuilder: Done!");
    return 0;
}

int main(int argc, char **argv) {
    std::shared_ptr<spdlog::logger> logger = VulkLogger::CreateLogger("BuildTool");
    string args = "Args: ";
    bool verbose = false;
    for (int i = 0; i < argc; i++) {
        args += argv[i];
        args += " ";
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    if (verbose) {
        spdlog::set_level(spdlog::level::trace);
        VULK_SET_TRACE_LOG_LEVEL();
        logger->info("Verbose mode enabled");
    }
    logger->info("Starting BuildTool {}", args);

    CLI::App app{"BuildTool for compiling vulk resources like shaders/pipelines etc."};
    app.add_flag("-v, --verbose", verbose, "be verbose");

    CLI::App *pipeline = app.add_subcommand("pipeline", "build the pipeline file");
    fs::path builtShadersDir;
    pipeline->add_option("builtShadersDir", builtShadersDir, "Directory where the built shaders are located (for reading).");
    fs::path pipelineFileOut;
    pipeline->add_option("pipelineFileOut", pipelineFileOut, "Directory where the pipelines are built to."); // Corrected here
    fs::path pipelineFileIn;
    pipeline->add_option("pipelineFileIn", pipelineFileIn, "Pipeline file to build."); // Corrected here
    pipeline->callback(
        [&builtShadersDir, &pipelineFileOut, &pipelineFileIn, &verbose]() { pipelineBuilder(builtShadersDir, pipelineFileOut, pipelineFileIn, verbose); });

    CLI::App *glsl = app.add_subcommand("GenVulkShaderEnums", "generate GLSL includes");
    fs::path outFile;
    glsl->add_option("outFile", outFile, "Directory where the GLSL includes are generated to.");
    glsl->callback([&outFile]() {
        outFile.make_preferred();
        glslShaderEnumsGenerator(outFile);
    });

    // CLI::App *scene = app.add_subcommand("scene", "build the scene file");
    // fs::path sceneFileIn;
    // scene->add_option("sceneFileIn", sceneFileIn, "Scene file to build.");
    // fs::path sceneOutDir;
    // scene->add_option("sceneOutDir", sceneOutDir, "Directory where the scenes are built to.");
    // scene->add_flag("-v, --verbose", verbose, "be verbose");
    // scene->callback([&sceneFileIn, &sceneOutDir, &verbose]() { sceneBuilder(sceneFileIn, sceneOutDir, verbose); });

    // do it
    CLI11_PARSE(app, argc, argv);
}