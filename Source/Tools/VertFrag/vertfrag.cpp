#include "vertfrag.h"

namespace pegtl = tao::pegtl;

int main() {
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
    if (!pegtl::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder)) {
        std::cerr << "Error parsing input" << std::endl;
        return 1;
    }
    auto state = stateBuilder.build();
    state;
    std::cout << "Good bye!\n";
    return 0;
}
