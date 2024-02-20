#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>

#include "PipelineBuilder.h"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("PipelineBuilder");
    program.add_argument("builtShadersDir").help("directory where the shaders are (assumes subdirs of vert/geom/frag etc.)");
    program.add_argument("pipelineOutDir").help("resulting pipeline file to write out to");
    program.parse_args(argc, argv);
    auto builtShadersDir = fs::path(program.get<std::string>("builtShadersDir")).lexically_normal();
    auto pipelineOutDir = fs::path(program.get<std::string>("pipelineOutDir")).lexically_normal();
    if (!fs::exists(builtShadersDir)) {
        std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
        return 1;
    }
    if (!fs::exists(pipelineOutDir.parent_path())) {
        std::cerr << "Pipeline output directory does not exist: " << pipelineOutDir.parent_path() << std::endl;
        return 1;
    }

    std::cout << "Shaders Dir: " << builtShadersDir << std::endl;
    std::cout << "Pipeline Out: " << pipelineOutDir << std::endl;

    PipelineBuilder::buildPipelinesFromMetadata(builtShadersDir, pipelineOutDir);
    return 0;
}