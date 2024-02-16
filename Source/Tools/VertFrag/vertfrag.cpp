#include <iostream>
#include <string>

#include "Vulk/VulkShaderEnums.h"
#include "tao/pegtl.hpp"

namespace p = tao::pegtl;

// @ubo(XformsUBO xformsUBO, ModelXform modelUBO)
// void vert(Pos inPos, Norm inNorm, Tan inTan, TexCoord inTex)
// {
//     mat4 worldXform = xform.world * modelUBO.xform;
//     gl_Position = xform.proj * xform.view * worldXform *
//     vec4(inPosition, 1.0); outTexCoord = inTexCoord; outPos = vec3(worldXform
//     * vec4(inPosition, 1.0)); outNorm = vec3(worldXform * vec4(inNormal,
//     0.0)); outTangent = vec3(worldXform * vec4(inTangent, 0.0));
// }

// @ubo(EyePos eyePos, Lights lights, MaterialUBO materialUBO)
// @sampler(TextureSampler texSampler, TextureSampler normSampler)
// void frag(in Pos fragPos, in TexCoord fragTexCoord)
// {
//     vec4 tex = texture(texSampler, fragTexCoord);
//     vec3 norm = vec3(texture(normSampler, fragTexCoord));
//     outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos,
//     lightBuf.light.pos, fragPos, lightBuf.light.color, true);
// }

namespace vertfrag {
    using namespace tao::pegtl;

    struct ShaderDecl {
        struct UBO {
            VulkShaderUBOBindings type;
            std::string name;
        };
        std::vector<UBO> ubos;
    };

    struct State {
        ShaderDecl vert, frag;
    };

    class ShaderDeclBuilder {
        std::vector<VulkShaderUBOBindings> uboBindings;
        std::vector<std::string> uboNames;
        std::vector<VulkShaderTextureBindings> textureBindings;
        std::vector<std::string> textureNames;
        std::vector<VertBindingLocations> vertBindings;
        std::vector<std::string> vertBindingNames;
        std::string funcBody;

      public:
        std::string funcName;
        void addUBOBinding(VulkShaderUBOBindings binding) {
            uboBindings.push_back(binding);
        }
        void addUBOName(const std::string &name) {
            uboNames.push_back(name);
        }
        void addSamplerBinding(VulkShaderTextureBindings binding) {
            textureBindings.push_back(binding);
        }
        void addSamplerName(const std::string &name) {
            textureNames.push_back(name);
        }
        void addVertBinding(VertBindingLocations binding) {
            vertBindings.push_back(binding);
        }
        void addVertBindingName(const std::string &name) {
            vertBindingNames.push_back(name);
        }
        void addFunctionName(const std::string &name) {
            funcName = name;
        }
        void addFunctionBody(const std::string &bodyIn) {
            funcBody = bodyIn;
        }
        ShaderDecl build() {
            assert(uboBindings.size() == uboNames.size());
            assert(textureBindings.size() == textureNames.size());
            assert(vertBindings.size() == vertBindingNames.size());
            assert(funcBody.size() > 0);
            ShaderDecl s;
            for (size_t i = 0; i < uboBindings.size(); i++) {
                s.ubos.push_back({uboBindings[i], uboNames[i]});
            }
            return s;
        }
    };
    class StateBuilder {
        State s;

      public:
        std::unique_ptr<ShaderDeclBuilder> b;
        StateBuilder() : b(std::make_unique<ShaderDeclBuilder>()) {
        }
        void buildShaderDecl() {
            if (b->funcName == "vert")
                s.vert = b->build();
            else if (b->funcName == "frag")
                s.frag = b->build();
            else
                throw std::runtime_error("Unknown shader type");

            b = std::make_unique<ShaderDeclBuilder>(); // Reset the builder
        }
        State build() {
            return s;
        }
    };

    template <typename Rule> struct control : p::normal<Rule> {
        // This method is called when a rule fails to match.
        template <typename Input, typename... States> static void raise(const Input &in, States &&...) {
            std::cerr << "Error: Failed to match rule '" << p::demangle<Rule>()
                      << "' at position " + std::to_string(in.position().byte) + ", character: '" + in.peek_char() + "'\n";
            throw p::parse_error("Error: Failed to match rule", in);
        }
    };

    // Rule to match comments
    struct single_line_comment : p::seq<p::star<p::space>, p::two<'/'>, p::until<p::eolf>> {};
    struct multi_line_comment : p::if_must<p::seq<p::one<'/'>, p::one<'*'>>, p::until<p::seq<p::one<'*'>, p::one<'/'>>>> {};
    struct skip : sor<space, single_line_comment, multi_line_comment> {};

    // Class template for user-defined actions that does
    // nothing by default.

    template <typename Rule> struct action {};

    // pegtl requires explicit whitespace matching
    struct spaces : p::plus<p::space> {};     // Rule for spaces
    struct opt_spaces : p::star<p::space> {}; // Rule for spaces

    // Specialisation of the user-defined action to do
    // something when the 'ubo' rule succeeds; is called
    // with the portion of the input that matched the rule.

    // @ubo(XformsUBO xformsUBO, ModelXform modelUBO)
    // @ubo(ubo_type ubo_name, ubo_type ubo_name)
    struct ubo_keyword : TAO_PEGTL_KEYWORD("@ubo") {}; // p::string<'@', 'u', 'b', 'o'> {};
    struct ubo_type : p::plus<p::identifier> {};
    struct ubo_name : p::plus<p::identifier> {};
    struct ubo_param : p::seq<ubo_type, spaces, ubo_name> {}; // Define the parameter rule
    struct ubo_declaration : p::seq<ubo_keyword, p::one<'('>, p::list<ubo_param, p::one<','>>, p::one<')'>> {};

    struct shader_param_type : p::plus<p::identifier> {};
    struct shader_param_name : p::plus<p::identifier> {};
    struct shader_param : p::seq<opt_spaces, shader_param_type, spaces, shader_param_name, opt_spaces> {}; // Define the parameter rule

    struct not_brace : p::not_one<'{', '}'> {};
    struct content; // forward decl
    struct brace_pair : p::seq<p::one<'{'>, p::until<p::one<'}'>, content>> {};
    struct content : p::star<p::sor<not_brace, brace_pair>> {};
    struct function_body : brace_pair {};

    struct shader_name : p::plus<p::identifier> {};
    struct shader_start : seq<TAO_PEGTL_KEYWORD("void"), spaces, shader_name> {};
    struct shader_func_decl : p::seq<shader_start, p::one<'('>, p::list<shader_param, p::one<','>>, p::one<')'>, p::star<p::space>, function_body> {};

    // seq<ubo_declaration, shader_func_decl> {};
    struct shader_decl : seq<ubo_declaration, spaces, shader_func_decl> {};
    struct vertfrag_body : star<sor<shader_decl, skip>> {};
    // struct grammar : p::must<vertfrag_body, p::eof> {};
    struct grammar : p::must<shader_decl, p::eof> {};

    template <> struct action<ubo_type> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addUBOBinding(VulkShaderEnums::uboBindingFromString(in.string()));
        }
    };
    template <> struct action<ubo_name> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addUBOName(in.string());
        }
    };
    template <> struct action<shader_param_type> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addVertBinding(VulkShaderEnums::vertBindingFromString(in.string()));
        }
    };
    template <> struct action<shader_param_name> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addVertBindingName(in.string());
        }
    };
    template <> struct action<shader_name> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addFunctionName(in.string());
        }
    };
    template <> struct action<function_body> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addFunctionBody(in.string());
            s.buildShaderDecl();
        }
    };

} // namespace vertfrag

int main() {
    vertfrag::StateBuilder stateBuilder;
    p::memory_input<> in(R"(@ubo(XformsUBO xformsIn)
    void vert(Pos inPos, Normal inNorm, Tangent inTan, TexCoord inTex)
    {
        mat4 worldXform = xform.world * modelUBO.xform;
        gl_Position = xform.proj * xform.view * worldXform *
        vec4(inPosition, 1.0); outTexCoord = inTexCoord; outPos = vec3(worldXform
        * vec4(inPosition, 1.0)); outNorm = vec3(worldXform * vec4(inNormal,
        0.0)); outTangent = vec3(worldXform * vec4(inTangent, 0.0));
    })",
                         "vertfrag input");

    // p::argv_input in(argv, 1);
    if (!p::parse<vertfrag::grammar, vertfrag::action, vertfrag::control>(in, stateBuilder)) {
        std::cerr << "Error parsing input" << std::endl;
        return 1;
    }
    auto state = stateBuilder.build();
    state;
    std::cout << "Good bye "
              << "\n";
    return 0;
}
