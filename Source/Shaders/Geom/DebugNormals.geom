#version 450

#include "common.glsl"

XFORMS_UBO(xform);

layout(location = LayoutLocation_Normal) in vec3 inNorm[1];

layout (points) in; // tells the shader that it will be receiving points, could also be lines, triangles, etc.

// outputs: we're going to output just one line with 2 vertices
layout (line_strip, max_vertices = 2) out;
layout(location = LayoutLocation_Position) out vec3 outPos;
layout(location = LayoutLocation_Normal) out vec3 outNorm;

void main() {    
    vec4 worldPos = xform.world * gl_in[0].gl_Position;
    vec4 worldNorm = xform.world * vec4(inNorm[0], 0.0);

    // Output the original point
    outPos = vec3(worldPos);
    outNorm = vec3(worldNorm);
    gl_Position = xform.proj * xform.view * worldPos;
    EmitVertex();

    // Output a second point offset by the normal
    outPos = worldPos.xyz; // use the same position as the original point so we don't throw off the color calculation
    outNorm = worldNorm.xyz;
    gl_Position = xform.proj * xform.view * (worldPos + .1f * worldNorm);
    EmitVertex();

    EndPrimitive();
}
