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
// #include "VulkResourceMetadata_generated.h"

#define FlatBufEnumSaveMinimal(EnumType)                                                                                                                       \
    template <class Archive> std::string save_minimal(Archive const &archive, EnumType const &type) {                                                          \
        return EnumName##EnumType(type);                                                                                                                       \
    }                                                                                                                                                          \
    template <class Archive> void load_minimal(Archive const &archive, EnumType &type, std::string const &value) {                                             \
        static std::unordered_map<std::string, EnumType> enumMap;                                                                                              \
        static std::once_flag flag;                                                                                                                            \
        std::call_once(flag, [&]() {                                                                                                                           \
            const char *const *vals = EnumNames##EnumType();                                                                                                   \
            for (int i = EnumType##_MIN; *vals[i]; i++) {                                                                                                      \
                EnumType enumValue = static_cast<EnumType>(i);                                                                                                 \
                enumMap[vals[i]] = enumValue;                                                                                                                  \
            }                                                                                                                                                  \
        });                                                                                                                                                    \
        type = enumMap[value.c_str()];                                                                                                                         \
    }

namespace cereal {
    FlatBufEnumSaveMinimal(MeshDefType);
    FlatBufEnumSaveMinimal(VulkShaderUBOBinding);
    FlatBufEnumSaveMinimal(VulkShaderSSBOBinding);
    FlatBufEnumSaveMinimal(VulkShaderTextureBinding);

    template <class Archive> std::string save_minimal(const Archive &, const VkShaderStageFlagBits &m) {
        return DescriptorSetDef::shaderStageToStr(m);
    }

    template <class Archive> void load_minimal(const Archive &, VkShaderStageFlagBits &m, const std::string &value) {
        m = DescriptorSetDef::getShaderStageFromStr(value);
    }

    // this errors because this is just an int, not a real type
    // template <class Archive> std::string save_minimal(const Archive &, const VkColorComponentFlags &m) {
    //     string mask;
    //     if (m & VK_COLOR_COMPONENT_R_BIT)
    //         mask += "R";
    //     if (m & VK_COLOR_COMPONENT_G_BIT)
    //         mask += "G";
    //     if (m & VK_COLOR_COMPONENT_B_BIT)
    //         mask += "B";
    //     if (m & VK_COLOR_COMPONENT_A_BIT)
    //         mask += "A";
    //     return mask;
    // }

    // template <class Archive> void load_minimal(const Archive &, VkColorComponentFlags &mask, const std::string &colorMask) {
    //     VkColorComponentFlags mask = 0;
    //     if (colorMask.find("R") != string::npos)
    //         mask |= VK_COLOR_COMPONENT_R_BIT;
    //     if (colorMask.find("G") != string::npos)
    //         mask |= VK_COLOR_COMPONENT_G_BIT;
    //     if (colorMask.find("B") != string::npos)
    //         mask |= VK_COLOR_COMPONENT_B_BIT;
    //     if (colorMask.find("A") != string::npos)
    //         mask |= VK_COLOR_COMPONENT_A_BIT;
    // }
} // namespace cereal

namespace cereal {
    template <class Archive> void serialize(Archive &archive, glm::vec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::vec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::vec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::ivec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::ivec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::ivec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::uvec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::uvec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::uvec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::dvec2 &v) {
        archive(v.x, v.y);
    }
    template <class Archive> void serialize(Archive &archive, glm::dvec3 &v) {
        archive(v.x, v.y, v.z);
    }
    template <class Archive> void serialize(Archive &archive, glm::dvec4 &v) {
        archive(v.x, v.y, v.z, v.w);
    }

    // glm matrices serialization
    template <class Archive> void serialize(Archive &archive, glm::mat2 &m) {
        archive(m[0], m[1]);
    }
    template <class Archive> void serialize(Archive &archive, glm::dmat2 &m) {
        archive(m[0], m[1]);
    }
    template <class Archive> void serialize(Archive &archive, glm::mat3 &m) {
        archive(m[0], m[1], m[2]);
    }
    template <class Archive> void serialize(Archive &archive, glm::mat4 &m) {
        archive(m[0], m[1], m[2], m[3]);
    }
    template <class Archive> void serialize(Archive &archive, glm::dmat4 &m) {
        archive(m[0], m[1], m[2], m[3]);
    }

    template <class Archive> void serialize(Archive &archive, glm::quat &q) {
        archive(q.x, q.y, q.z, q.w);
    }
    template <class Archive> void serialize(Archive &archive, glm::dquat &q) {
        archive(q.x, q.y, q.z, q.w);
    }

} // namespace cereal

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

struct MyData {
    int x;
    int y;
    glm::vec3 vec;
    // Serialize the MyData struct for JSON output
    template <class Archive> void serialize(Archive &ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(vec));
    }
};

struct MaterialDef2 {
    std::string name;
    std::string mapKa; // Ambient texture map
    // std::string mapKd;     // Diffuse texture map
    // std::string mapKs;     // Specular texture map
    // std::string mapNormal; // Normal map (specified as 'bump' in the file)
    // float Ns;              // Specular exponent (shininess)
    // float Ni;              // Optical density (index of refraction)
    // float d;               // Transparency (dissolve)
    // glm::vec3 Ka;          // Ambient color
    // glm::vec3 Kd;          // Diffuse color
    // glm::vec3 Ks;          // Specular color

    template <class Archive> void serialize(Archive &ar) {
        ar(name, mapKa);
    }
};

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

int pipelineBuilder(fs::path builtShadersDir, fs::path pipelineOutDir, fs::path pipelineFileIn, bool verbose) {
    LOG("PipelineBuilder: STUBBED OUT FIX!!!! Building pipeline from file: {}", pipelineFileIn.string());
    //     if (!fs::exists(builtShadersDir)) {
    //         std::cerr << "Shaders directory does not exist: " << builtShadersDir << std::endl;
    //         return 1;
    //     }
    //     if (!fs::exists(pipelineOutDir.parent_path())) {
    //         std::cerr << "Pipeline output directory does not exist: " << pipelineOutDir.parent_path() << std::endl;
    //         return 1;
    //     }
    //     if (!fs::exists(pipelineFileIn)) {
    //         std::cerr << "Pipeline file does not exist: " << pipelineFileIn << std::endl;
    //         return 1;
    //     }

    //     if (verbose) {
    //         std::cout << "Shaders Dir: " << builtShadersDir << std::endl;
    //         std::cout << "Pipeline Out Dir: " << pipelineOutDir << std::endl;
    //         std::cout << "Processing pipeline: " << pipelineFileIn << std::endl;
    //     }
    //     try {
    //         PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineOutDir, pipelineFileIn);
    //     } catch (std::exception &e) {
    //         std::cerr << "PipelineBuilder: Error: " << e.what() << std::endl;
    //         return 1;
    //     }
    //     PipelineBuilder::buildPipelineFromFile(builtShadersDir, pipelineOutDir, pipelineFileIn);

    //     if (verbose) {
    //         std::cout << "PipelineBuilder: Done!\n";
    //     }

    return 0;
}

int main(int argc, char **argv) {
    CLI::App app{"BuildTool for compiling vulk resources like shaders/pipelines etc."};

    CLI::App *pipeline = app.add_subcommand("pipeline", "build the pipeline file");
    fs::path builtShadersDir;
    pipeline->add_option("builtShadersDir", builtShadersDir, "Directory where the built shaders are located (for reading).");
    fs::path pipelineOutDir;
    pipeline->add_option("pipelineOutDir", pipelineOutDir, "Directory where the pipelines are built to."); // Corrected here
    fs::path pipelineFileIn;
    pipeline->add_option("pipelineFileIn", pipelineFileIn, "Pipeline file to build."); // Corrected here
    bool verbose = false;
    pipeline->add_flag("-v, --verbose", verbose, "be verbose");

    pipeline->callback(
        [&builtShadersDir, &pipelineOutDir, &pipelineFileIn, &verbose]() { pipelineBuilder(builtShadersDir, pipelineOutDir, pipelineFileIn, verbose); });

    CLI11_PARSE(app, argc, argv);
}