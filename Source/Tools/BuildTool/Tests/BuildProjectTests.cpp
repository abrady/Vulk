#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch.hpp>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <filesystem>
#include <iostream>
#include <memory>

#include <filesystem>

#include "Vulk/VulkUtil.h"

#include "../BuildPipeline.h"
#include "../BuildProject.h"

TEST_CASE("build a project") {
    std::filesystem::path projectFile = std::filesystem::path(__FILE__).parent_path() / "TestProjDir" / "test.proj";
    fs::path buildDir                 = "./TestProjBuildDir";
    std::filesystem::remove_all(buildDir);
    fs::create_directories(buildDir);
    buildProjectDef(projectFile, buildDir);

    vulk::cpp2::ProjectDef def;
    readDefFromFile((buildDir / "test.proj").string(), def);
    /*

    Directory: C:\open\Github\Vulk\build\Source\Tools\BuildTool\Tests\Debug\TestProjBuildDir\Assets\common

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---           5/24/2024 10:29 PM           2573 common.glsl
-a---           6/10/2024  9:04 PM           1646 VulkShaderEnums_generated.glsl

    Directory: C:\open\Github\Vulk\build\Source\Tools\BuildTool\Tests\Debug\TestProjBuildDir\Assets\Pipelines

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---           6/10/2024 12:24 AM             92 test.pipeline

    Directory: C:\open\Github\Vulk\build\Source\Tools\BuildTool\Tests\Debug\TestProjBuildDir\Assets\Scenes

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---            6/9/2024 11:02 PM           1401 test.scene

    Directory: C:\open\Github\Vulk\build\Source\Tools\BuildTool\Tests\Debug\TestProjBuildDir\Assets\Shaders\frag

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---           6/10/2024  9:04 PM          30524 test.fragspv

    Directory: C:\open\Github\Vulk\build\Source\Tools\BuildTool\Tests\Debug\TestProjBuildDir\Assets\Shaders\Pipelines

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---           6/10/2024  9:04 PM            554 test

    Directory: C:\open\Github\Vulk\build\Source\Tools\BuildTool\Tests\Debug\TestProjBuildDir\Assets\Shaders\vert

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a---           6/10/2024  9:04 PM           9096 test.vertspv
}*/
    REQUIRE(def.get_name() == "TestProject");
    REQUIRE(fs::exists(buildDir / "Assets" / "common" / "common.glsl"));
    REQUIRE(fs::exists(buildDir / "Assets" / "common" / "VulkShaderEnums_generated.glsl"));
    REQUIRE(fs::exists(buildDir / "Assets" / "Pipelines" / "test.pipeline"));
    REQUIRE(fs::exists(buildDir / "Assets" / "Pipelines" / "DebugNormals.pipeline"));
    REQUIRE(fs::exists(buildDir / "Assets" / "Scenes" / "test.scene"));
    REQUIRE(fs::exists(buildDir / "Assets" / "Shaders" / "frag" / "test.fragspv"));
    REQUIRE(fs::exists(buildDir / "Assets" / "Shaders" / "vert" / "test.vertspv"));
    REQUIRE(fs::exists(buildDir / "Assets" / "Materials" / "test" / "test.mtl"));

    vulk::cpp2::PipelineDef pipelineDef;
    readDefFromFile((buildDir / "Assets" / "Pipelines" / "test.pipeline").string(), pipelineDef);
    REQUIRE(pipelineDef.get_name() == "test");
    vulk::cpp2::DescriptorSetDef descriptorSetDef = pipelineDef.get_descriptorSetDef();
    REQUIRE(descriptorSetDef.get_uniformBuffers().size() == 2);
}
