#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

#include "CLI/CLI.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <filesystem>

#include "Vulk/VulkUtil.h"

#include "PipelineBuilder.h"

// Initialize a shared logger instance
inline std::shared_ptr<spdlog::logger> &GetLogger() {
    static std::shared_ptr<spdlog::logger> logger;
    static std::once_flag flag;
    std::call_once(flag, []() {
        // Create a color console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // Create a file sink
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logfile.log", true);

        // Combine the sinks into a list
        spdlog::sinks_init_list sink_list = {file_sink, console_sink};

        // Create a logger with both sinks
        logger = std::make_shared<spdlog::logger>("BuildTool", sink_list.begin(), sink_list.end());

        // Register it as the default logger
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        // Set the logger's level (e.g., info, warning, error)
        logger->set_level(spdlog::level::info);
    });
    return logger;
}

#define LOG(...) GetLogger()->info(__VA_ARGS__)
#define WARN(...) GetLogger()->warn(__VA_ARGS__)
#define ERROR(...) GetLogger()->error(__VA_ARGS__)
namespace fs = std::filesystem;

int sceneBuilder(fs::path sceneFileIn, fs::path sceneOutDir, bool verbose) {
    LOG("SceneBuilder: Building scene from file: {}", sceneFileIn.string());
    if (!fs::exists(sceneFileIn)) {
        ERROR("Scene file does not exist: {}", sceneFileIn.string());
        return 1;
    }
    if (!fs::exists(sceneOutDir)) {
        ERROR("Scene output directory does not exist: {}", sceneOutDir.string());
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
    LOG("PipelineBuilder: Building pipeline from file: {}", pipelineFileIn.string());
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
    // CLI11_PARSE(app, argc, argv);
}