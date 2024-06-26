#version 450

#include "common.glsl"


layout (points) in; // tells the shader that it will be receiving points, could also be lines, triangles, etc.
layout(location = VulkShaderLocation_Pos) in vec3 inWorldPos[1];
layout(location = VulkShaderLocation_Pos2) in vec4 outProjPos[1];
layout(location = VulkShaderLocation_Normal) in vec3 inWorldNorm[1];

// outputs: we're going to output just one line with 2 vertices
layout (line_strip, max_vertices = 2) out;
layout(location = VulkShaderLocation_Pos) out vec3 outWorldPos;
layout(location = VulkShaderLocation_Normal) out vec3 outWorldNorm;

layout(push_constant) uniform pickPushConstants {
    uint objectID;
} pc;


void main() {    
    // Output the original point
    gl_Position = gl_in[0].gl_Position;
    outWorldPos = inWorldPos[0];
    outWorldNorm = inWorldNorm[0];
    EmitVertex();

    // Output a second point offset by the normal
    outWorldPos = inWorldPos[0];
    outWorldNorm = inWorldNorm[0];
    gl_Position = outProjPos[0];
    EmitVertex();

    EndPrimitive();
}
