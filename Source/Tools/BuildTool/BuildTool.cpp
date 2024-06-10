#include <filesystem>
#include <iostream>
#include <memory>

#include "CLI/CLI.hpp"

#include <filesystem>

#include "BuildPipeline.h"
#include "Vulk/VulkLogger.h"
#include "Vulk/VulkUtil.h"

namespace fs = std::filesystem;

extern void glslShaderEnumsGenerator(fs::path outFile, bool verbose);
extern void buildProjectDef(const fs::path project_file_path, fs::path buildDir);

void pipelineBuilder(fs::path builtShadersDir, fs::path pipelineFileOut, fs::path pipelineFileIn, bool verbose) {
    auto logger = VulkLogger::CreateLogger("PipelineBuilder");
    if (verbose) {
        logger->set_level(spdlog::level::trace);
    }

    logger->info("PipelineBuilder: Building pipeline from file: {}", pipelineFileIn.string());
    if (!fs::exists(builtShadersDir)) {
        logger->error("Shaders directory does not exist: {}", builtShadersDir.string());
        throw CLI::ValidationError("Shaders directory does not exist: " + builtShadersDir.string());
    }
    if (!fs::exists(pipelineFileOut.parent_path())) {
        logger->error("Pipeline output directory does not exist: {}", pipelineFileOut.parent_path().string());
        throw CLI::ValidationError("Pipeline output directory does not exist: " + pipelineFileOut.parent_path().string());
    }
    if (!fs::exists(pipelineFileIn)) {
        logger->error("Pipeline file does not exist: {}", pipelineFileIn.string());
        throw CLI::ValidationError("Pipeline file does not exist: " + pipelineFileIn.string());
    }

    logger->trace("Shaders Dir: {}, Pipeline Out Dir: {}, Processing pipeline: {}", builtShadersDir.string(), pipelineFileOut.string(), pipelineFileIn.string());

    try {
        PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineFileOut, pipelineFileIn);
    } catch (std::exception& e) {
        logger->error("PipelineBuilder: Error: {}", e.what());
        throw CLI::Error("PipelineBuilder", "Error: " + string(e.what()));
    } catch (...) {
        VulkException e("Unknown error while processing " + pipelineFileIn.string());
        logger->error("PipelineBuilder: Unknown error while processing {}", e.what());
        throw CLI::Error("PipelineBuilder", "win32 Error: " + string(e.what()));
    }

    logger->trace("PipelineBuilder: Done!");
}

int main(int argc, char** argv) {
    // SetUnhandledExceptionFilter(exceptionFilter);
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

    CLI::App* pipeline = app.add_subcommand("pipeline", "build the pipeline file");
    fs::path builtShadersDir;
    pipeline->add_option("builtShadersDir", builtShadersDir, "Directory where the built shaders are located (for reading).");
    fs::path pipelineFileOut;
    pipeline->add_option("pipelineFileOut", pipelineFileOut, "Directory where the pipelines are built to."); // Corrected here
    fs::path pipelineFileIn;
    pipeline->add_option("pipelineFileIn", pipelineFileIn, "Pipeline file to build."); // Corrected here
    pipeline->callback([&builtShadersDir, &pipelineFileOut, &pipelineFileIn, &verbose]() {
        pipelineBuilder(builtShadersDir, pipelineFileOut, pipelineFileIn, verbose);
    });

    CLI::App* glsl = app.add_subcommand("GenVulkShaderEnums", "generate GLSL includes");
    fs::path outFile;
    glsl->add_option("outFile", outFile, "Directory where the GLSL includes are generated to.");
    glsl->callback([&outFile, verbose]() {
        outFile.make_preferred();
        glslShaderEnumsGenerator(outFile, verbose);
    });

    // buildProjectDef(const fs::path project_file_path, fs::path buildDir, fs::path generatedHeaderDir)
    CLI::App* project = app.add_subcommand("project", "build the project file");
    fs::path projectFileIn;
    project->add_option("projectFileIn", projectFileIn, "Project file to build.");
    fs::path projectOutDir;
    project->add_option("projectOutDir", projectOutDir, "Directory where the projects are built to.");
    project->callback([&projectFileIn, &projectOutDir]() {
        buildProjectDef(projectFileIn, projectOutDir);
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