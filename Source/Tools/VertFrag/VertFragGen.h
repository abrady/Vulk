#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include "VertFragParser.h"
#include "Vulk/VulkResourceMetadata.h"

class VertFragGen {
  private:
    vertfrag::State state;

    static std::string uboDecl(VulkShaderUBOBindings ubo, std::string name) {
        switch (ubo) {
        case VulkShaderUBOBinding_Xforms:
            return std::format("XFORMS_UBO({});", name);
        case VulkShaderUBOBinding_Lights:
            return std::format("LIGHTS_UBO({});", name);
        case VulkShaderUBOBinding_EyePos:
            return std::format("EYEPOS_UBO({});", name);
        case VulkShaderUBOBinding_ModelXform:
            return std::format("MODELXFORM_UBO({});", name);
        case VulkShaderUBOBinding_MaterialUBO:
            return std::format("MATERIAL_UBO({});", name);
        case VulkShaderUBOBinding_DebugNormals:
            return std::format("DEBUGNORMALS_UBO({});", name);
        case VulkShaderUBOBinding_DebugTangents:
            return std::format("DEBUGNORMALS_UBO({});", name);
        default:
            throw std::runtime_error("Unknown UBO binding");
        };
    }

    static std::string samplerDecl(VulkShaderTextureBindings sampler, std::string name) {
        switch (sampler) {
        case VulkShaderTextureBinding_TextureSampler:
            return std::format("layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D {};", name);
        case VulkShaderTextureBinding_TextureSampler2:
            return std::format("layout(binding = VulkShaderBinding_TextureSampler2) uniform sampler2D {};", name);
        case VulkShaderTextureBinding_TextureSampler3:
            return std::format("layout(binding = VulkShaderBinding_TextureSampler3) uniform sampler2D {};", name);
        case VulkShaderTextureBinding_NormalSampler:
            return std::format("layout(binding = VulkShaderBinding_NormalSampler) uniform sampler2D {};", name);
        default:
            throw std::runtime_error("Unknown sampler binding");
        }
    }

    static std::string vertBindingLocationDecl(VulkVertBindingLocation binding, std::string name, std::string prefix = "in") {
        switch (binding) {
        case VulkVertBindingLocation_ColorBinding:
            return std::format("layout(location = LayoutLocation_Color) {} vec4 {};", prefix, name);
        case VulkVertBindingLocation_PosBinding:
            return std::format("layout(location = LayoutLocation_Position) {} vec3 {};", prefix, name);
        case VulkVertBindingLocation_NormalBinding:
            return std::format("layout(location = LayoutLocation_Normal) {} vec3 {};", prefix, name);
        case VulkVertBindingLocation_TangentBinding:
            return std::format("layout(location = LayoutLocation_Tangent) {} vec3 {};", prefix, name);
        case VulkVertBindingLocation_TexCoordBinding:
            return std::format("layout(location = LayoutLocation_TexCoord) {} vec2 {};", prefix, name);
        case VulkVertBindingLocation_HeightBinding:
            return std::format("layout(location = LayoutLocation_Height) {} vec3 {};", prefix, name);
        case VulkVertBindingLocation_Pos2Binding:
            return std::format("layout(location = LayoutLocation_Position2) {} vec3 {};", prefix, name);
        default:
            throw std::runtime_error("Unknown vertex binding: " + std::to_string(binding));
        }
    }

  public:
    VertFragGen(vertfrag::State &&state) : state(std::move(state)) {
    }

    std::string declToShaderBody(vertfrag::ShaderDecl const &decl) const {
        std::string shader;
        for (auto const &ubo : decl.ubos)
            shader += uboDecl(ubo.type, ubo.name) + "\n";

        for (auto const &sampler : decl.samplers)
            shader += samplerDecl(sampler.type, sampler.name) + "\n";

        for (auto const &inBinding : decl.inBindings)
            shader += vertBindingLocationDecl(inBinding.type, inBinding.name, "in") + "\n";

        for (auto const &outBinding : decl.outBindings)
            shader += vertBindingLocationDecl(outBinding.type, outBinding.name, "out") + "\n";

        return shader;
    }

    void writeShaders(filesystem::path shaderDirBase, filesystem::path pipelineDir) const {
        filesystem::create_directories(shaderDirBase);
        bool hasVertShader = false;
        bool hasFragShader = false;
        for (auto const &shader : state.shaderDecls) {
            std::string shaderBody = declToShaderBody(shader);
            std::ofstream out(shaderDirBase / shader.name / std::format("{}.{}", state.name, shader.name));
            out << shaderBody;
            out.close();

            if (shader.name == "vert")
                hasVertShader = true;
            else if (shader.name == "frag")
                hasFragShader = true;
        }

        std::ofstream out(pipelineDir / std::format("{}.pipeline", state.name));

        std::string descriptorSet;
        for (auto const &shader : state.shaderDecls) {
            if (!descriptorSet.empty())
                descriptorSet += ",\n";
            if (shader.name == "vert")
                descriptorSet += "vertex: {\n";
            else if (shader.name == "frag") {
                descriptorSet += "fragment: {\n";
            } else {
                throw std::runtime_error("Unknown shader type");
            }

            std::string ubos;
            for (auto const &ubo : shader.ubos) {
                if (!ubos.empty())
                    ubos += ", ";
                ubos += shaderUBOBindingToStr(ubo.type);
            }
            std::string samplers;
            for (auto const &sampler : shader.samplers) {
                if (!samplers.empty())
                    samplers += ", ";
                samplers += shaderTextureBindingToStr(sampler.type);
            }

            if (!ubos.empty())
                descriptorSet += std::format("uniformBuffers: [{}]\n", ubos);

            if (!samplers.empty()) {
                if (!ubos.empty())
                    descriptorSet += ", ";
                descriptorSet += std::format("imageSamplers: [{}]\n", samplers);
            }

            descriptorSet += "}";
        }
        out << std::format(R"(
{{
    "version": 1,
    "name": "{}",
    "vertexShader": "{}",
    "fragmentShader": "{}",
    "vertexInputBinding": 0,
    "descriptorSet": {{
        {}
    }}
}}
)",
                           state.name, state.name, state.name, descriptorSet);
        out.close();
    }
};