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

namespace fs = std::filesystem;

int sceneBuilder(fs::path sceneFileIn, fs::path sceneOutDir) {
    VULK_LOG("SceneBuilder: Building scene from file: {}", sceneFileIn.string());
    if (!fs::exists(sceneFileIn)) {
        VULK_ERR("Scene file does not exist: {}", sceneFileIn.string());
        return 1;
    }
    if (!fs::exists(sceneOutDir)) {
        VULK_ERR("Scene output directory does not exist: {}", sceneOutDir.string());
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

int pipelineBuilder(fs::path builtShadersDir, fs::path pipelineFileOut, fs::path pipelineFileIn, bool verbose) {
    VULK_LOG("PipelineBuilder: Building pipeline from file: {}", pipelineFileIn.string());
    if (!fs::exists(builtShadersDir)) {
        std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
        return 1;
    }
    if (!fs::exists(pipelineFileOut.parent_path())) {
        std::cerr << "Pipeline output directory does not exist: " << pipelineFileOut.parent_path() << std::endl;
        return 1;
    }
    if (!fs::exists(pipelineFileIn)) {
        std::cerr << "Pipeline file does not exist: " << pipelineFileIn << std::endl;
        return 1;
    }

    if (verbose) {
        std::cout << "Shaders Dir: " << builtShadersDir << std::endl;
        std::cout << "Pipeline Out Dir: " << pipelineFileOut << std::endl;
        std::cout << "Processing pipeline: " << pipelineFileIn << std::endl;
    }
    try {
        PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineFileOut, pipelineFileIn);
    } catch (std::exception &e) {
        std::cerr << "PipelineBuilder: Error: " << e.what() << std::endl;
        return 1;
    }
    PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineFileOut, pipelineFileIn);

    if (verbose) {
        std::cout << "PipelineBuilder: Done!\n";
    }

    return 0;
}

int main(int argc, char **argv) {
    CLI::App app{"BuildTool for compiling vulk resources like shaders/pipelines etc."};

    CLI::App *pipeline = app.add_subcommand("pipeline", "build the pipeline file");
    fs::path builtShadersDir;
    pipeline->add_option("builtShadersDir", builtShadersDir, "Directory where the built shaders are located (for reading).");
    fs::path pipelineFileOut;
    pipeline->add_option("pipelineFileOut", pipelineFileOut, "Directory where the pipelines are built to."); // Corrected here
    fs::path pipelineFileIn;
    pipeline->add_option("pipelineFileIn", pipelineFileIn, "Pipeline file to build."); // Corrected here
    bool verbose = false;
    pipeline->add_flag("-v, --verbose", verbose, "be verbose");
    pipeline->callback(
        [&builtShadersDir, &pipelineFileOut, &pipelineFileIn, &verbose]() { pipelineBuilder(builtShadersDir, pipelineFileOut, pipelineFileIn, verbose); });

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