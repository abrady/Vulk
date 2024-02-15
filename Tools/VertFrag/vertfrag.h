#pragma once

#include "pegtl.hpp"
/*
@ubo(XformsUBO xformsUBO, ModelXform modelUBO)
void vert(in Pos inPos, in Norm inNorm, in Tan inTan, in TexCoord inTex, out  )
{
    mat4 worldXform = xform.world * modelUBO.xform;
    gl_Position = xform.proj * xform.view * worldXform * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    outPos = vec3(worldXform * vec4(inPosition, 1.0));
    outNorm = vec3(worldXform * vec4(inNormal, 0.0));
    outTangent = vec3(worldXform * vec4(inTangent, 0.0));
}

@ubo(EyePos eyePos, Lights lights, MaterialUBO materialUBO)
@sampler(TextureSampler texSampler, TextureSampler normSampler)
void frag(in Pos fragPos, in TexCoord fragTexCoord)
{
    vec4 tex = texture(texSampler, fragTexCoord);
    vec3 norm = vec3(texture(normSampler, fragTexCoord));
    outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos, lightBuf.light.pos, fragPos, lightBuf.light.color, true);
}
*/
using namespace tao::pegtl;

struct ubo_keyword : string<'@', 'u', 'b', 'o'>
{
};
struct sampler_keyword : string<'@', 's', 'a', 'm', 'p', 'l', 'e', 'r'>
{
};

struct id : plus<alnum>
{
};

struct ubo_declaration : seq<ubo_keyword, one<'('>, id, id, one<')'>>
{
};
struct sampler_declaration : seq<sampler_keyword, one<'('>, id, id, one<')'>>
{
};

struct function_declaration : seq<string<'v', 'o', 'i', 'd'>, id, one<'('>, star<id>, one<')'>>
{
};
struct grammar : plus<sor<ubo_declaration, sampler_declaration, function_declaration>>
{
};

template <typename Rule>
struct my_action : nothing<Rule>
{
};

template <>
struct my_action<id>
{
    template <typename Input>
    static void apply(const Input &in, std::string &out)
    {
        out = in.string();
    }
};