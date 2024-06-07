#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include <filesystem>
#include <iostream>
#include <memory>

#include <filesystem>

#include "Vulk/VulkUtil.h"

#include "../BuildPipeline.h"

namespace fs = std::filesystem;

TEST_CASE("basic serialization") {
    vulk::cpp2::BuiltPipelineDef def;
    def.version_ref() = 1;
    def.name_ref() = "TestPipeline";
    def.vertShaderName_ref() = "test.vert.spv";
    def.geomShaderName_ref() = "test.geom.spv";
    def.fragShaderName_ref() = "test.frag.spv";
    writeDefToFile("foo.pipeline", def);

    vulk::cpp2::BuiltPipelineDef def2;
    readDefFromFile("foo.pipeline", def2);
    REQUIRE(def == def2);
}