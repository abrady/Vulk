#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_EyePos) uniform EyePos {
    vec3 eyePos;
} eyePosUBO;


layout(location = LayoutLocation_Position) in vec3 inPos;
layout(location = LayoutLocation_Normal) in vec3 inNorm;
layout(location = 0) out vec4 outColor;
void main()
{
    vec3 norm = normalize(inNorm); // interpolated normals may be de-normlized.
    // v0 of this: just turn the normal into a color. cons: not really providing useful info except to distinguish between different normals
    // vec3 normalColor = normalize(norm);  // Normalize the normal
    // normalColor = normalColor * 0.5 + 0.5; // Scale and bias to fit 0 to 1
    // outColor = vec4(normalColor, 1.0);

    // v1 of this: turn the normal into a color, but also take into account the eye position
    vec3 viewDir = normalize(eyePosUBO.eyePos - inPos); // Calculate view direction
    float facingRatio = dot(viewDir, norm); // Calculate dot product

    // Scale and bias the facing ratio to fit 0 to 1
    facingRatio = facingRatio * 0.5 + 0.5;

    // Use the facing ratio to interpolate between a "cold" and "warm" color
    vec3 coldColor = vec3(0.0, 0.0, 1.0); 
    vec3 warmColor = vec3(0.0, 1.0, 0.0); 
    vec3 normalColor = mix(coldColor, warmColor, facingRatio);

    //outColor = vec4(normalColor, 1.0);    
    outColor = vec4(1,1,0,1);
}
