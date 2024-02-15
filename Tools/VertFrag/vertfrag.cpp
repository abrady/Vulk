#include <iostream>
#include <fstream>

#include "vertfrag.h"

int main()
{
    std::string code = R"(
        @ubo(XformsUBO xformsUBO, ModelXform modelUBO)
        void vert(in Pos inPos, in Norm inNorm, in Tan inTan, in TexCoord inTex, out )
        {
            // function body here...
        }
        @ubo(EyePos eyePos, Lights lights, MaterialUBO materialUBO)
        @sampler(TextureSampler texSampler, TextureSampler normSampler)
        void frag(in Pos fragPos, in TexCoord fragTexCoord)
        {
            // function body here...
        }
    )";
    std::string out;
    tao::pegtl::memory_input<> in(code, "code");
    tao::pegtl::parse<grammar, my_action>(in, out);
    std::cout << "qParsed identifiers: " << out << std::endl;
    return 0;
}

/*int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    const std::string inputFile = argv[1];
    const std::string outputFile = argv[2];

    std::cout << "File processed successfully!\n";
    return 0;
}
*/