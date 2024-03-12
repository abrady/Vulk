#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>

#include "PipelineBuilder.h"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("PipelineBuilder");
    program.add_argument("builtShadersDir").help("directory where the shaders are (assumes subdirs of vert/geom/frag etc.)");
    program.add_argument("pipelineOutDir").help("resulting pipeline file to write out to");
    program.add_argument("pipelineFileIn").help("process this pipeline file");
    program.add_argument("-v", "--verbose").help("be verbose").default_value(false);
    program.parse_args(argc, argv);
    bool verbose = program.get<bool>("-v");
    if (verbose) {
        std::string cmdline;
        for (int i = 0; i < argc; i++) {
            if (i > 0) {
                cmdline += " ";
            }
            cmdline += argv[i];
        }
        std::cout << "PipelineBuilder: " << cmdline << std::endl;
    }
    auto builtShadersDir = fs::path(program.get<std::string>("builtShadersDir")).lexically_normal();
    auto pipelineOutDir = fs::path(program.get<std::string>("pipelineOutDir")).lexically_normal();
    auto pipelineFileIn = fs::path(program.get<std::string>("pipelineFileIn")).lexically_normal();
    if (!fs::exists(builtShadersDir)) {
        std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
        return 1;
    }
    if (!fs::exists(pipelineOutDir.parent_path())) {
        std::cerr << "Pipeline output directory does not exist: " << pipelineOutDir.parent_path() << std::endl;
        return 1;
    }
    if (!fs::exists(pipelineFileIn)) {
        std::cerr << "Pipeline file does not exist: " << pipelineFileIn << std::endl;
        return 1;
    }

    if (verbose) {
        std::cout << "Shaders Dir: " << builtShadersDir << std::endl;
        std::cout << "Pipeline Out Dir: " << pipelineOutDir << std::endl;
        std::cout << "Processing pipeline: " << pipelineFileIn << std::endl;
    }
    try {
        PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineOutDir, pipelineFileIn);
    } catch (std::exception &e) {
        std::cerr << "PipelineBuilder: Error: " << e.what() << std::endl;
        return 1;
    }
    PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineOutDir, pipelineFileIn);

    if (verbose) {
        std::cout << "PipelineBuilder: Done!\n";
    }

    return 0;
}