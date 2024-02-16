#include "../VertFrag.h" // Include your parser header

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

namespace pegtl = tao::pegtl;

TEST_CASE("Vertfrag Tests") {
    // Define your tests here
    SECTION("Test Parsing") {
        vertfrag::StateBuilder stateBuilder;
        pegtl::memory_input<> in(R"(
    // some comments!

    @ubo(XformsUBO xformsIn)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {
        mat4 worldXform = xform.world * modelUBO.xform;
        gl_Position = xform.proj * xform.view * worldXform *
        vec4(inPosition, 1.0); outTexCoord = inTexCoord; outPos = vec3(worldXform
        * vec4(inPosition, 1.0)); outNorm = vec3(worldXform * vec4(inNormal,
        0.0)); outTangent = vec3(worldXform * vec4(inTangent, 0.0));
    }
    )",
                                 "vertfrag input");

        // argv_input in(argv, 1);
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        state;
        REQUIRE(state.vert.ubos.size() == 1);
        REQUIRE(state.vert.ubos[0].type == VulkShaderUBOBinding_Xforms);
        REQUIRE(state.vert.ubos[0].name == "xformsIn");
    }
}
