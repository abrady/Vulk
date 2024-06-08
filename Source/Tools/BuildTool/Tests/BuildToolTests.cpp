#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include <filesystem>

#include "Vulk/VulkUtil.h"

#include "../BuildPipeline.h"

namespace fs = std::filesystem;

TEST_CASE("basic Pipeline serialization") {
    vulk::cpp2::PipelineDef def;
    def.version_ref() = 1;
    def.name_ref() = "TestPipeline";
    def.vertShader_ref() = "test.vert.spv";
    def.geomShader_ref() = "test.geom.spv";
    def.fragShader_ref() = "test.frag.spv";
    def.primitiveTopology_ref() = vulk::cpp2::VulkPrimitiveTopology::LineListWithAdjacency;
    writeDefToFile("test.pipeline", def);

    vulk::cpp2::PipelineDef def2;
    readDefFromFile("test.pipeline", def2);
    REQUIRE(def == def2);
}

TEST_CASE("basic Scene serialization") {
    vulk::cpp2::SceneDef def;
    def.name_ref() = "TestScene";
    // def.camera_ref()->eye_ref().emplace = {1.0, 2.0, 3.0};
    vulk::cpp2::Vec3& cam = def.camera_ref()->eye_ref().value();
    cam.x_ref() = 1.0;
    cam.y_ref() = 2.0;
    cam.z_ref() = 3.0;

    vulk::cpp2::Vec3& lookAt = def.camera_ref()->lookAt_ref().value();
    lookAt.x_ref() = 4.1;
    lookAt.y_ref() = 5.2;
    lookAt.z_ref() = 6.3;
    def.camera_ref()->nearClip_ref() = 0.1;
    def.camera_ref()->farClip_ref() = 100.0;

    writeDefToFile("test.scene", def);

    vulk::cpp2::SceneDef def2;
    readDefFromFile("test.scene", def2);
    REQUIRE(def == def2);
}