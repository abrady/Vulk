#version 450

#include "common.glsl"

layout(location = VulkShaderLocation_TexCoord) out vec2 outTexCoord;

// normalized device coordinates are in the range [-1, 1] for x and y
// for the lighting phase we just need a quad stretching from -1 to 1
// this assumes a triangle strip with 4 vertices
//      1 -->  3
//     / \    /
//    /   \  / 
//   0 <-- 2  
// e.g. bl, tl, br, tr
// 0 -> 0,0 -> -1,-1
// 1 -> 2,0 ->  1,-1
// 2 -> 0,2 -> -1, 1
// 3 -> 2,2 ->  1, 1
// order: bl, br, tl, tr
// vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) - 1.0; // or divide by 2 or shift right by 1...
// boo, wrong winding order for Vulkan. I'm sure there's some fancy way to do this with bit manipulation, but 
// I want this to be readable...
// can I turn off backface fulling for a subpass?
void main() {
    vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0), // bl
        vec2(-1.0,  1.0), // tl
        vec2( 1.0, -1.0), // br
        vec2( 1.0,  1.0)  // tr
    );    
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
    outTexCoord = vertices[gl_VertexIndex] * 0.5 + 0.5;
}


