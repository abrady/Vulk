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
#include "VulkShaderEnums_types.h"

namespace fs = std::filesystem;

// #include <DbgHelp.h>
// #include <dbghelp.h>
// #include <windows.h>
// #pragma comment(lib, "DbgHelp.lib")

// void printStackTrace(CONTEXT *context) {
//     STACKFRAME64 stackframe = {};
//     stackframe.AddrPC.Offset = context->Rip;
//     stackframe.AddrPC.Mode = AddrModeFlat;
//     stackframe.AddrFrame.Offset = context->Rbp;
//     stackframe.AddrFrame.Mode = AddrModeFlat;
//     stackframe.AddrStack.Offset = context->Rsp;
//     stackframe.AddrStack.Mode = AddrModeFlat;

//     HANDLE process = GetCurrentProcess();
//     HANDLE thread = GetCurrentThread();

//     SymInitialize(process, NULL, TRUE);

//     while (StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &stackframe, context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
//         DWORD64 displacement = 0;
//         char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
//         PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
//         symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
//         symbol->MaxNameLen = MAX_SYM_NAME;

//         if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
//             std::cout << symbol->Name << std::endl;
//         }
//     }

//     SymCleanup(process);
// }

// LONG WINAPI exceptionFilter(struct _EXCEPTION_POINTERS *exceptionPointers) {
//     std::cout << "Access violation at address: " << exceptionPointers->ExceptionRecord->ExceptionAddress << std::endl;
//     printStackTrace(exceptionPointers->ContextRecord);
//     return EXCEPTION_EXECUTE_HANDLER;
// }

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

    for (auto [key, value] : vulk::_VulkShaderBinding_VALUES_TO_NAMES) {
        out << "const int VulkShaderBinding_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkShaderBinding_{} = {};", value, (int)key);
    }

    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    for (auto [key, value] : vulk::_VulkShaderLocation_VALUES_TO_NAMES) {
        out << "const int VulkShaderLocation_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkShaderLocation_{} = {};", value, (int)key);
    }

    // Write the light constants
    // Write the layout locations
    out << "\n// Shader Input Locations\n";
    for (auto [key, value] : vulk::_VulkLights_VALUES_TO_NAMES) {
        out << "const int VulkLights_" << value << " = " << (int)key << ";\n";
        logger->trace("const int VulkLights_{} = {};", value, (int)key);
    }

    out << "\n";

    out.close();

    // // throw CLI::Success(); // not really necessary
}

void pipelineBuilder(fs::path builtShadersDir, fs::path pipelineFileOut, fs::path pipelineFileIn, bool verbose) {
    auto logger = VulkLogger::CreateLogger("PipelineBuilder");
    if (verbose) {
        logger->set_level(spdlog::level::trace);
    }

    vulk::BuiltPipelineDef def;
    def.version = 1;
    def.name = "TestPipeline";
    def.vertShaderName = "test.vert.spv";
    def.geomShaderName = "test.geom.spv";
    def.fragShaderName = "test.frag.spv";
    writeDefToFile("foo.pipeline", def);

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