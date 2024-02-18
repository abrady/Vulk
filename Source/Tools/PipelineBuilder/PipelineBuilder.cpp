#include <filesystem>
#include <iostream>

#include <argparse/argparse.hpp>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("PipelineBuilder");
    program.add_argument("pipelineIn").help("pipeline file to read from");
    program.add_argument("shadersDir").help("directory where the shaders are (assumes subdirs of vert/geom/frag etc.)");
    program.add_argument("pipelineOut").help("resulting pipeline file to write out to");
    program.parse_args(argc, argv);
    fs::path pipelineIn = fs::path(program.get<std::string>("pipelineIn")).lexically_normal();
    auto shadersDir = fs::path(program.get<std::string>("shadersDir")).lexically_normal();
    auto pipelineOut = fs::path(program.get<std::string>("pipelineIn")).lexically_normal();
    if (!fs::exists(pipelineIn)) {
        std::cerr << "Pipeline file does not exist: " << pipelineIn << std::endl;
        return 1;
    }
    if (!fs::exists(shadersDir)) {
        std::cerr << "Shaders directory does not exist: " << shadersDir << std::endl;
        return 1;
    }
    if (!fs::exists(pipelineOut.parent_path())) {
        std::cerr << "Pipeline output directory does not exist: " << pipelineOut.parent_path() << std::endl;
        return 1;
    }

    std::cout << "Pipeline In: " << pipelineIn << std::endl;
    std::cout << "Shaders Dir: " << shadersDir << std::endl;
    std::cout << "Pipeline Out: " << pipelineOut << std::endl;
    return 0;
}