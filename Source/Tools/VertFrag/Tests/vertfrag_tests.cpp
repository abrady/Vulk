#include "../VertFragParser.h" // Include your parser header

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

namespace pegtl = tao::pegtl;

TEST_CASE("Vertfrag Tests") {
    // Define your tests here
    SECTION("Test Parsing Simple") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(@out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {})",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &vert = state.shaderDecls[0];
        REQUIRE(vert.ubos.size() == 0);
        REQUIRE(vert.inBindings.size() == 4);
        REQUIRE(vert.inBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.inBindings[0].name == "inPos");
        REQUIRE(vert.inBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.inBindings[1].name == "inNorm");
        REQUIRE(vert.inBindings[2].type == VulkVertBindingLocation_TangentBinding);
        REQUIRE(vert.inBindings[2].name == "inTan");
        REQUIRE(vert.inBindings[3].type == VulkVertBindingLocation_TexCoordBinding);
        REQUIRE(vert.inBindings[3].name == "inTex");
        REQUIRE(vert.outBindings.size() == 2);
        REQUIRE(vert.outBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.outBindings[0].name == "outPos");
        REQUIRE(vert.outBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.outBindings[1].name == "outNorm");
    }
    SECTION("Test Parsing With UBO") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(@ubo(XformsUBO xformsIn, ModelXform modelIn)
    @out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {})",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &vert = state.shaderDecls[0];
        REQUIRE(vert.ubos.size() == 2);
        REQUIRE(vert.ubos[0].type == VulkShaderUBOBinding_Xforms);
        REQUIRE(vert.ubos[0].name == "xformsIn");
        REQUIRE(vert.ubos[1].type == VulkShaderUBOBinding_ModelXform);
        REQUIRE(vert.ubos[1].name == "modelIn");
        REQUIRE(vert.outBindings.size() == 2);
        REQUIRE(vert.outBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.outBindings[0].name == "outPos");
        REQUIRE(vert.outBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.outBindings[1].name == "outNorm");
    }
    SECTION("Test Parsing With Vert and Frag") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(@out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {}
    @out(Pos outPos, Normal outNorm)
    void frag(Pos inPos, Normal inNorm)
    {})",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &frag = state.shaderDecls[1];
        REQUIRE(frag.ubos.size() == 0);
        REQUIRE(frag.inBindings.size() == 2);
        REQUIRE(frag.inBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(frag.inBindings[0].name == "inPos");
        REQUIRE(frag.inBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(frag.inBindings[1].name == "inNorm");
    }
    SECTION("Test Parsing With Vert and Frag with whitespace") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(
            
    @out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {}

    @out(Pos outPos, Normal outNorm)
    void frag(Pos inPos, Normal inNorm)
    {}
    
    )",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &frag = state.shaderDecls[1];
        REQUIRE(frag.ubos.size() == 0);
        REQUIRE(frag.inBindings.size() == 2);
        REQUIRE(frag.inBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(frag.inBindings[0].name == "inPos");
        REQUIRE(frag.inBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(frag.inBindings[1].name == "inNorm");
    }

    SECTION("Test Frag Parsing") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(
            
    @ubo(EyePos eyePos, Lights lights, MaterialUBO materialUBO)
    @sampler(TextureSampler texSampler, NormalSampler normSampler)
    @out(Color outColor)
    void frag(Pos fragPos, TexCoord fragTexCoord)
    {
        vec4 tex = texture(texSampler, fragTexCoord);
        vec3 norm = vec3(texture(normSampler, fragTexCoord));
        outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos,
        lightBuf.light.pos, fragPos, lightBuf.light.color, true);
    }
    
    )",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &frag = state.shaderDecls[0];
        REQUIRE(frag.ubos.size() == 3);
        REQUIRE(frag.ubos[0].type == VulkShaderUBOBinding_EyePos);
        REQUIRE(frag.ubos[0].name == "eyePos");
        REQUIRE(frag.ubos[1].type == VulkShaderUBOBinding_Lights);
        REQUIRE(frag.ubos[1].name == "lights");
        REQUIRE(frag.ubos[2].type == VulkShaderUBOBinding_MaterialUBO);
        REQUIRE(frag.ubos[2].name == "materialUBO");
        REQUIRE(frag.samplers.size() == 2);
        REQUIRE(frag.samplers[0].type == VulkShaderTextureBinding_TextureSampler);
        REQUIRE(frag.samplers[0].name == "texSampler");
        REQUIRE(frag.samplers[1].type == VulkShaderTextureBinding_NormalSampler);
        REQUIRE(frag.samplers[1].name == "normSampler");
        REQUIRE(frag.inBindings.size() == 2);
        REQUIRE(frag.inBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(frag.inBindings[0].name == "fragPos");
        REQUIRE(frag.inBindings[1].type == VulkVertBindingLocation_TexCoordBinding);
        REQUIRE(frag.inBindings[1].name == "fragTexCoord");
    }
    SECTION("Test Parsing") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(@ubo(XformsUBO xformsIn, ModelXform modelIn)
    @out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {}
    @ubo(XformsUBO xformsIn, ModelXform modelIn)
    @out(Pos outPos, Normal outNorm)
    void frag(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {})",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
    }
    SECTION("Test Frag With Comments") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(
            
            // spaces

            /* 
            * and
            * comments
            */
            
            @ubo(XformsUBO xformsIn, ModelXform modelIn)
    @out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {
        mat4 worldXform = xform.world * modelUBO.xform;
        gl_Position = xform.proj * xform.view * worldXform *
        vec4(inPosition, 1.0); outTexCoord = inTexCoord; outPos = vec3(worldXform
        * vec4(inPosition, 1.0)); outNorm = vec3(worldXform * vec4(inNormal,
        0.0)); outTangent = vec3(worldXform * vec4(inTangent, 0.0));
    })",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &vert = state.shaderDecls[0];
        REQUIRE(vert.ubos.size() == 2);
        REQUIRE(vert.ubos[0].type == VulkShaderUBOBinding_Xforms);
        REQUIRE(vert.ubos[0].name == "xformsIn");
        REQUIRE(vert.ubos[1].type == VulkShaderUBOBinding_ModelXform);
        REQUIRE(vert.ubos[1].name == "modelIn");

        REQUIRE(vert.inBindings.size() == 4);
        REQUIRE(vert.inBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.inBindings[0].name == "inPos");
        REQUIRE(vert.inBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.inBindings[1].name == "inNorm");
        REQUIRE(vert.inBindings[2].type == VulkVertBindingLocation_TangentBinding);
        REQUIRE(vert.inBindings[2].name == "inTan");
        REQUIRE(vert.inBindings[3].type == VulkVertBindingLocation_TexCoordBinding);
        REQUIRE(vert.inBindings[3].name == "inTex");

        REQUIRE(vert.outBindings.size() == 2);
        REQUIRE(vert.outBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.outBindings[0].name == "outPos");
        REQUIRE(vert.outBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.outBindings[1].name == "outNorm"); //
    }
    SECTION("Test Frag/Vert With Comments") {
        vertfrag::StateBuilder stateBuilder("test");
        pegtl::memory_input<> in(R"(
            
            // spaces

    /* 
    * and
    * comments
    */
    @ubo(XformsUBO xformsIn, ModelXform modelIn)
    @out(Pos outPos, Normal outNorm)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {
        mat4 worldXform = xform.world * modelUBO.xform;
        gl_Position = xform.proj * xform.view * worldXform *
        vec4(inPosition, 1.0); outTexCoord = inTexCoord; outPos = vec3(worldXform
        * vec4(inPosition, 1.0)); outNorm = vec3(worldXform * vec4(inNormal,
        0.0)); outTangent = vec3(worldXform * vec4(inTangent, 0.0));
    }
    
    // some comments
    @ubo(EyePos eyePos, Lights lights, MaterialUBO materialUBO)
    @sampler(TextureSampler texSampler, TextureSampler normSampler)
    @out(Color outColor)
    void frag(Pos fragPos, TexCoord fragTexCoord)
    {
        vec4 tex = texture(texSampler, fragTexCoord);
        vec3 norm = vec3(texture(normSampler, fragTexCoord));
        outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos,
        lightBuf.light.pos, fragPos, lightBuf.light.color, true);
    }
    
    )",
                                 "vertfrag input");
        REQUIRE(pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder));
        auto state = stateBuilder.build();
        auto &vert = state.shaderDecls[0];
        REQUIRE(vert.ubos.size() == 2);
        REQUIRE(vert.ubos[0].type == VulkShaderUBOBinding_Xforms);
        REQUIRE(vert.ubos[0].name == "xformsIn");
        REQUIRE(vert.ubos[1].type == VulkShaderUBOBinding_ModelXform);
        REQUIRE(vert.ubos[1].name == "modelIn");

        REQUIRE(vert.inBindings.size() == 4);
        REQUIRE(vert.inBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.inBindings[0].name == "inPos");
        REQUIRE(vert.inBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.inBindings[1].name == "inNorm");
        REQUIRE(vert.inBindings[2].type == VulkVertBindingLocation_TangentBinding);
        REQUIRE(vert.inBindings[2].name == "inTan");
        REQUIRE(vert.inBindings[3].type == VulkVertBindingLocation_TexCoordBinding);
        REQUIRE(vert.inBindings[3].name == "inTex");

        REQUIRE(vert.outBindings.size() == 2);
        REQUIRE(vert.outBindings[0].type == VulkVertBindingLocation_PosBinding);
        REQUIRE(vert.outBindings[0].name == "outPos");
        REQUIRE(vert.outBindings[1].type == VulkVertBindingLocation_NormalBinding);
        REQUIRE(vert.outBindings[1].name == "outNorm"); //
    }
}
