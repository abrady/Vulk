#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include "Vulk/VulkShaderEnums.h"
#include "spirv_cross/spirv_glsl.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

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

TEST_CASE("PipelineBuilder Tests") { // Define your tests here
    SECTION("Test Basics") {
        auto spirvData = readSPIRVFile(fs::path(__FILE__).parent_path() / "vert" / "DebugNormals.vertspv");
        spirv_cross::CompilerGLSL glsl(std::move(spirvData));
        const spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        // For UBOs
        std::unordered_map<std::string, std::string> bindings;
        for (const spirv_cross::Resource &resource : resources.uniform_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            std::cout << "UBO: " << resource.name << " Set: " << set << " Binding: " << VulkShaderEnums::shaderBindingToString(binding) << std::endl;
            CHECK_FALSE(bindings.contains(resource.name));
            bindings[resource.name] = VulkShaderEnums::shaderBindingToString(binding);
        }
        CHECK(bindings["UniformBufferObject"] == "VulkShaderBinding_XformsUBO");
        CHECK(bindings["ModelXformUBO"] == "VulkShaderBinding_ModelXform");
        CHECK(bindings["DebugNormalsUBO"] == "VulkShaderBinding_DebugNormalsUBO");

        // For SBOs
        for (const spirv_cross::Resource &resource : resources.storage_buffers) {
            unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            std::cout << "SBO: " << resource.name << " Set: " << set << " Binding: " << VulkShaderEnums::shaderBindingToString(binding) << std::endl;
            CHECK_FALSE(bindings.contains(resource.name));
            bindings[resource.name] = VulkShaderEnums::shaderBindingToString(binding);
        }

        // Inputs
        std::unordered_map<std::string, unsigned> inputLocations;
        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            VulkVertBindingLocation location = (VulkVertBindingLocation)glsl.get_decoration(resource.id, spv::DecorationLocation);
            std::string loc = VulkShaderEnums::vertBindingLocationToString(location);
            std::cout << "Input: " << resource.name << " Location: " << loc << std::endl;
            CHECK_FALSE(inputLocations.contains(resource.name));
            inputLocations[resource.name] = location;
        }
        CHECK(inputLocations["inPosition"] == VulkVertBindingLocation_PosBinding);
        CHECK(inputLocations["inNormal"] == VulkVertBindingLocation_NormalBinding);
        CHECK(inputLocations["inTangent"] == VulkVertBindingLocation_TangentBinding);
        // Outputs
        std::unordered_map<std::string, unsigned> outputLocations;
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            VulkVertBindingLocation location = (VulkVertBindingLocation)glsl.get_decoration(resource.id, spv::DecorationLocation);
            std::string loc = VulkShaderEnums::vertBindingLocationToString(location);
            std::cout << "Output: " << resource.name << " Location: " << loc << std::endl;
            CHECK_FALSE(outputLocations.contains(resource.name));
            outputLocations[resource.name] = location;
        }
        CHECK(outputLocations["outWorldPos"] == VulkVertBindingLocation_PosBinding);
        CHECK(outputLocations["outWorldNorm"] == VulkVertBindingLocation_NormalBinding);
        CHECK(outputLocations["outProjPos"] == VulkVertBindingLocation_Pos2Binding);
    }
}
