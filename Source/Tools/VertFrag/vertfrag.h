#pragma once

#include "Vulk/VulkShaderEnums.h"
#include "tao/pegtl.hpp"
#include <iostream>
#include <string>

// @ubo(XformsUBO xformsUBO, ModelXform modelUBO)
// @out(Pos outPos, Norm outNorm, Tan outTangent, TexCoord outTexCoord)
// void vert(Pos inPos, Norm inNorm, Tan inTan, TexCoord inTex)
// {
//     mat4 worldXform = xform.world * modelUBO.xform;
//     gl_Position = xform.proj * xform.view * worldXform *
//     vec4(inPosition, 1.0); outTexCoord = inTexCoord; outPos = vec3(worldXform
//     * vec4(inPosition, 1.0)); outNorm = vec3(worldXform * vec4(inNormal,
//     0.0)); outTangent = vec3(worldXform * vec4(inTangent, 0.0));
//
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
        struct VertBindings {
            VulkVertBindingLocation type;
            std::string name;
        };

        std::vector<UBO> ubos;
        std::vector<VertBindings> inBindings;
        std::vector<VertBindings> outBindings;
    };

    struct State {
        std::vector<ShaderDecl> shaderDecls;
    };

    class ShaderDeclBuilder {
        std::vector<VulkShaderUBOBindings> uboBindings;
        std::vector<std::string> uboNames;
        std::vector<VulkShaderTextureBindings> textureBindings;
        std::vector<std::string> textureNames;
        std::vector<VulkVertBindingLocation> inBindings;
        std::vector<std::string> inBindingNames;
        std::vector<VulkVertBindingLocation> outBindings;
        std::vector<std::string> outBindingNames;
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
        void addInBinding(VulkVertBindingLocation binding) {
            inBindings.push_back(binding);
        }
        void addInBindingName(const std::string &name) {
            inBindingNames.push_back(name);
        }
        void addOutBinding(VulkVertBindingLocation binding) {
            outBindings.push_back(binding);
        }
        void addOutBindingName(const std::string &name) {
            outBindingNames.push_back(name);
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
            assert(inBindings.size() == inBindingNames.size());
            assert(outBindings.size() == outBindingNames.size());
            assert(funcBody.size() > 0);
            ShaderDecl s;
            for (size_t i = 0; i < uboBindings.size(); i++) {
                s.ubos.push_back({uboBindings[i], uboNames[i]});
            }
            for (size_t i = 0; i < inBindings.size(); i++) {
                s.inBindings.push_back({inBindings[i], inBindingNames[i]});
            }
            for (size_t i = 0; i < outBindings.size(); i++) {
                s.outBindings.push_back({outBindings[i], outBindingNames[i]});
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
            assert(b->funcName == "vert" || b->funcName == "frag");
            s.shaderDecls.push_back(b->build());
            b = std::make_unique<ShaderDeclBuilder>(); // Reset the builder
        }
        State build() {
            return s;
        }
    };

    template <typename Rule> struct control : normal<Rule> {
        // This method is called when a rule fails to match.
        template <typename Input, typename... States> static void raise(const Input &in, States &&...) {
            std::cerr << "Error: Failed to match rule '" << demangle<Rule>()
                      << "' at position " + std::to_string(in.position().byte) + ", character: '" + in.peek_char() + "'\n";
            throw parse_error("Error: Failed to match rule", in);
        }
    };

    // Rule to match comments
    struct single_line_comment : seq<star<space>, two<'/'>, until<eolf>> {};
    struct multi_line_comment : if_must<seq<one<'/'>, one<'*'>>, until<seq<one<'*'>, one<'/'>>>> {};
    struct skip : sor<space, single_line_comment, multi_line_comment> {};

    // Class template for user-defined actions that does
    // nothing by default.

    template <typename Rule> struct action {};

    // pegtl requires explicit whitespace matching
    struct spaces : plus<space> {};     // Rule for spaces
    struct opt_spaces : star<space> {}; // Rule for spaces

    // Specialisation of the user-defined action to do
    // something when the 'ubo' rule succeeds; is called
    // with the portion of the input that matched the rule.

    // @ubo(XformsUBO xformsUBO, ModelXform modelUBO)
    // @ubo(ubo_type ubo_name, ubo_type ubo_name)
    struct ubo_keyword : TAO_PEGTL_KEYWORD("@ubo") {};
    struct ubo_type : plus<identifier> {};
    struct ubo_name : plus<identifier> {};
    struct ubo_param : seq<ubo_type, spaces, ubo_name> {};
    struct ubo_declaration : seq<ubo_keyword, one<'('>, list<ubo_param, one<','>>, one<')'>> {};

    struct shader_param_type : plus<identifier> {};
    struct shader_param_name : plus<identifier> {};
    struct shader_param : seq<opt_spaces, shader_param_type, spaces, shader_param_name, opt_spaces> {};

    struct out_keyword : TAO_PEGTL_KEYWORD("@out") {};
    struct out_param_type : plus<identifier> {};
    struct out_param_name : plus<identifier> {};
    struct out_param : seq<opt_spaces, out_param_type, spaces, out_param_name, opt_spaces> {};
    struct out_declaration : seq<out_keyword, one<'('>, list<out_param, one<','>>, one<')'>> {};

    struct not_brace : not_one<'{', '}'> {};
    struct content; // forward decl
    struct brace_pair : seq<one<'{'>, until<one<'}'>, content>> {};
    struct content : star<sor<not_brace, brace_pair>> {};
    struct function_body : brace_pair {};

    struct shader_name : plus<identifier> {};
    struct shader_start : seq<TAO_PEGTL_KEYWORD("void"), spaces, shader_name> {};
    struct shader_func_decl : seq<shader_start, one<'('>, list<shader_param, one<','>>, one<')'>, star<space>, function_body> {};

    // seq<ubo_declaration, shader_func_decl> {};
    struct shader_decl : seq<ubo_declaration, spaces, out_declaration, spaces, shader_func_decl> {};
    struct vertfrag_body : star<sor<shader_decl, skip>> {};
    // struct grammar : must<vertfrag_body, eof> {};
    struct grammar : must<shader_decl, eof> {};

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
            s.b->addInBinding(VulkShaderEnums::vertBindingFromString(in.string()));
        }
    };
    template <> struct action<shader_param_name> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addInBindingName(in.string());
        }
    };
    template <> struct action<out_param_type> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addOutBinding(VulkShaderEnums::vertBindingFromString(in.string()));
        }
    };
    template <> struct action<out_param_name> {
        template <typename ParseInput> static void apply(const ParseInput &in, StateBuilder &s) {
            s.b->addOutBindingName(in.string());
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