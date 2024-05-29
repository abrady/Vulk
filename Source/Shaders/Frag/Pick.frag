#version 450

layout(push_constant) uniform pickPushConstants {
    uint objectID;
} pc;

layout(location = 0) out uint outObjectID;

void main() {
    outObjectID = pc.objectID;
}