#version 450

#include "common.glsl"

// layout(push_constant) uniform pickPushConstants {
//     uint objectID;
// } pc;

// layout(location = VulkShaderLocation_Pos) in vec3 inPos;
layout(location = 0) out vec4 outColor;
layout(location = VulkShaderLocation_TexCoord) in vec2 inTexCoord;
void main()
{
    float axis = inTexCoord.x;
    // we pack the color information into the uvs
    // we do an interpolation in the shader: 0 = red, .5 = green, 1 = blue
    vec3 red = vec3(1.0, 0.0, 0.0);   // Red color
    vec3 green = vec3(0.0, 1.0, 0.0); // Green color
    vec3 blue = vec3(0.0, 0.0, 1.0);  // Blue color
    // Ensure the inputValue is clamped between 0 and 1
    float t = clamp(axis, 0.0, 1.0);
    // Determine the color by interpolating between red, green, and blue
    vec3 color;
    if (t < 0.5) {
        // Interpolate from red to green when t is in [0, 0.5]
        color = mix(red, green, t * 2.0); // Scale t to [0, 1] for this segment
    } else {
        // Interpolate from green to blue when t is in [0.5, 1]
        color = mix(green, blue, (t - 0.5) * 2.0); // Scale t to [0, 1] for this segment
    }    
    outColor = vec4(color, 1.0);
}
