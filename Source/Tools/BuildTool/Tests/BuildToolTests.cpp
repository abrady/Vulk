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

TEST_CASE("misc serialization test") {
    SECTION("CameraDef") {
        vulk::cpp2::CameraDef def;
        def.eye_ref()->x_ref() = 1.0;
        def.eye_ref()->y_ref() = 2.0;
        def.eye_ref()->z_ref() = 3.0;
        def.lookAt_ref()->x_ref() = 4.0;
        def.lookAt_ref()->y_ref() = 5.0;
        def.lookAt_ref()->z_ref() = 6.0;
        def.nearClip_ref() = 0.1;
        def.farClip_ref() = 100.0;
        writeDefToFile("test.camera", def);

        vulk::cpp2::CameraDef def2;
        readDefFromFile("test.camera", def2);
        REQUIRE(def == def2);
    }

    SECTION("GeoMeshDef") {
        vulk::cpp2::GeoMeshDef def;
        auto& sphere = def.set_sphere();
        sphere.radius_ref() = 1.0;
        sphere.numSubdivisions_ref() = 3;
        REQUIRE(def.get_sphere().get_radius() == 1.0);

        writeDefToFile("test.geomesh", def);

        vulk::cpp2::GeoMeshDef def2;
        readDefFromFile("test.geomesh", def2);
        REQUIRE(def2.get_sphere().get_radius() == 1.0);
        REQUIRE(def == def2);
    }

    SECTION("ModelDef") {
        vulk::cpp2::ModelDef def;
        def.name_ref() = "TestModel";
        def.mesh_ref() = "TestMesh";
        def.material_ref() = "TestMaterial";
        def.meshDefType_ref() = vulk::cpp2::MeshDefType::Mesh;
        vulk::cpp2::GeoSphereDef& sphere = def.geoMesh_ref()->set_sphere();
        sphere.radius_ref() = 1.0;
        sphere.numSubdivisions_ref() = 3;
        writeDefToFile("test.model", def);

        vulk::cpp2::ModelDef def2;
        readDefFromFile("test.model", def2);
        REQUIRE(def == def2);
    }

    SECTION("ProjectDef") {
        vulk::cpp2::ProjectDef def;
        def.name_ref() = "TestProject";
        vulk::cpp2::SceneDef sceneDef;
        sceneDef.name_ref() = "TestScene";
        // sceneDef.camera_ref()->eye_ref().emplace = {1.0, 2.0, 3.0};
        vulk::cpp2::Vec3& cam = sceneDef.camera_ref()->eye_ref().value();
        cam.x_ref() = 1.0;
        cam.y_ref() = 2.0;
        cam.z_ref() = 3.0;
        vulk::cpp2::Vec3& lookAt = sceneDef.camera_ref()->lookAt_ref().value();
        lookAt.x_ref() = 4.1;
        lookAt.y_ref() = 5.2;
        lookAt.z_ref() = 6.3;
        sceneDef.camera_ref()->nearClip_ref() = 0.1;
        sceneDef.camera_ref()->farClip_ref() = 100.0;
        def.scenes_ref()["TestScene"] = sceneDef;

        writeDefToFile("test.project", def);

        vulk::cpp2::ProjectDef def2;
        readDefFromFile("test.project", def2);
        REQUIRE(def == def2);
    }
}

TEST_CASE("make sure our json definitions still work") {
    SECTION("ModelDef") {
        std::string jsonDef = R"(
{
    "name": "Skull0",
    "mesh": "Skull1",
    "material": "Skull2"
}
)";
        vulk::cpp2::ModelDef def;
        apache::thrift::SimpleJSONSerializer::deserialize(jsonDef, def);
        REQUIRE(def.get_name() == "Skull0");
        REQUIRE(def.get_mesh() == "Skull1");
        REQUIRE(def.get_material() == "Skull2");
    }
    SECTION("PipelineDef") {
        std::string jsonDef = R"(
{
    "version": 1,
    "name": "LitModel",
    "vertShader": "BasicPassthru",
    "fragShader": "LitModel"
}
)";
        vulk::cpp2::PipelineDef def;
        apache::thrift::SimpleJSONSerializer::deserialize(jsonDef, def);
        REQUIRE(def.get_version() == 1);
        REQUIRE(def.get_name() == "LitModel");
        REQUIRE(def.get_vertShader() == "BasicPassthru");
        REQUIRE(def.get_fragShader() == "LitModel");
    }
    SECTION("CameraDef") {
        std::string jsonDef = R"(
{
        "eye": {
            "x": 1,
            "y": 2,
            "z": 3
        },
        "lookAt": {
            "x": 4,
            "y": 5,
            "z": 6
        },
        "nearClip": 0.1,
        "farClip": 20
    }
)";
        vulk::cpp2::CameraDef def;
        apache::thrift::SimpleJSONSerializer::deserialize(jsonDef, def);
        REQUIRE(def.get_nearClip() == 0.1);
        REQUIRE(def.get_farClip() == 20);
        REQUIRE(def.get_eye().get_x() == 1);
        REQUIRE(def.get_eye().get_y() == 2);
        REQUIRE(def.get_eye().get_z() == 3);
        REQUIRE(def.get_lookAt().get_x() == 4);
        REQUIRE(def.get_lookAt().get_y() == 5);
        REQUIRE(def.get_lookAt().get_z() == 6);
    }
    SECTION("SceneDef") {
        std::string jsonDef = R"(
{
    "name": "PBR2",
    "camera": {
        "eye": {
            "x": 1,
            "y": 2,
            "z": 3
        },
        "lookAt": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "nearClip": 0.1,
        "farClip": 20
    },
    "lights": [
        {
            "name": "Light1",
            "type": 2,
            "pos": {
                "x": 0,
                "y": -4,
                "z": 0
            },
            "color": {
                "x": 1,
                "y": 1,
                "z": 1
            }
        }
    ],
    "actors": [
        {
            "name": "HerringboneFloor",
            "pipeline": "PBR2",
            "inlineModel": {
                "version": 1,
                "name": "SphereModel1",
                "meshDefType": 1,
                "geoMesh": {
                  "sphere": {
                      "type": 1,
                      "radius": 1,
                      "numSubdivisions": 3
                  }
                },
                "material": "herringbone-flooring-bl"
            },
            "xform": {
                "pos": {
                    "x": 1,
                    "y": 2,
                    "z": 3
                },
                "rot": {
                    "x": 4,
                    "y": 5,
                    "z": 6
                }
            }
        }
    ]
}
)";
        vulk::cpp2::SceneDef def;
        apache::thrift::SimpleJSONSerializer::deserialize(jsonDef, def);
        REQUIRE(def.get_name() == "PBR2");
        REQUIRE(def.get_camera().get_nearClip() == 0.1);
        REQUIRE(def.get_camera().get_farClip() == 20);
        REQUIRE(def.get_camera().get_eye().get_x() == 1);
        REQUIRE(def.get_camera().get_eye().get_y() == 2);
        REQUIRE(def.get_camera().get_eye().get_z() == 3);
        REQUIRE(def.get_camera().get_lookAt().get_x() == 0);
        REQUIRE(def.get_camera().get_lookAt().get_y() == 0);
        REQUIRE(def.get_camera().get_lookAt().get_z() == 0);
        REQUIRE(def.get_lights().size() == 1);
        REQUIRE(def.get_lights()[0].get_type() == vulk::cpp2::LightType::Spot);
        REQUIRE(def.get_lights()[0].get_pos().get_x() == 0);
        REQUIRE(def.get_lights()[0].get_pos().get_y() == -4);
        REQUIRE(def.get_lights()[0].get_pos().get_z() == 0);
        REQUIRE(def.get_lights()[0].get_color().get_x() == 1);
        REQUIRE(def.get_lights()[0].get_color().get_y() == 1);
        REQUIRE(def.get_lights()[0].get_color().get_z() == 1);
        REQUIRE(def.get_actors().size() == 1);
        auto actor0 = def.get_actors()[0];
        REQUIRE(actor0.get_name() == "HerringboneFloor");
        REQUIRE(actor0.get_pipeline() == "PBR2");
        auto actor0Model = actor0.get_inlineModel();
        REQUIRE(actor0Model.get_name() == "SphereModel1");
        REQUIRE(actor0Model.get_meshDefType() == vulk::cpp2::MeshDefType::Mesh);
        vulk::cpp2::GeoMeshDef actor0GeoMesh = actor0Model.get_geoMesh();
        REQUIRE(actor0GeoMesh.get_sphere().get_radius() == 1);
        REQUIRE(actor0GeoMesh.get_sphere().get_numSubdivisions() == 3);
        REQUIRE(actor0Model.get_material() == "herringbone-flooring-bl");
        REQUIRE(actor0.get_xform().get_pos().get_x() == 1);
        REQUIRE(actor0.get_xform().get_pos().get_y() == 2);
        REQUIRE(actor0.get_xform().get_pos().get_z() == 3);
        REQUIRE(actor0.get_xform().get_rot().get_x() == 4);
        REQUIRE(actor0.get_xform().get_rot().get_y() == 5);
        REQUIRE(actor0.get_xform().get_rot().get_z() == 6);
    }
}