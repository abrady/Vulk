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

TEST_CASE("basic serialization") {
    vulk::cpp2::PipelineDef def;
    def.version_ref() = 1;
    def.name_ref() = "TestPipeline";
    def.vertShader_ref() = "test.vert.spv";
    def.geomShader_ref() = "test.geom.spv";
    def.fragShader_ref() = "test.frag.spv";
    def.primitiveTopology_ref() = vulk::cpp2::VulkPrimitiveTopology::LineListWithAdjacency;
    writeDefToFile("foo.pipeline", def);

    vulk::cpp2::PipelineDef def2;
    readDefFromFile("foo.pipeline", def2);
    REQUIRE(def == def2);
}