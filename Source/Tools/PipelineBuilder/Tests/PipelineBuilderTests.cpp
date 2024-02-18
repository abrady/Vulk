#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::vector<uint32_t> readSPIRVFile(fs::path filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    return buffer;
}

TEST_CASE("PipelineBuilder Tests") {
    // Define your tests here
    SECTION("Test Basics") {
        auto spirvData = readSPIRVFile(fs::path(__FILE__).parent_path() / "vert" / "DebugNormals.vertspv");
        spirv_cross::CompilerGLSL glsl(std::move(spirvData));
        const spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        // For UBOs
        for (const spirv_cross::Resource &resource : resources.uniform_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            std::cout << "UBO: " << resource.name << " Set: " << set << " Binding: " << binding << std::endl;
        }

        // For SBOs
        for (const spirv_cross::Resource &resource : resources.storage_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            std::cout << "SBO: " << resource.name << " Set: " << set << " Binding: " << binding << std::endl;
        }

        // Inputs
        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            unsigned location = glsl.get_decoration(resource.id, spv::DecorationLocation);
            std::cout << "Input: " << resource.name << " Location: " << location << std::endl;
        }

        // Outputs
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            unsigned location = glsl.get_decoration(resource.id, spv::DecorationLocation);
            std::cout << "Output: " << resource.name << " Location: " << location << std::endl;
        }
    }
}
