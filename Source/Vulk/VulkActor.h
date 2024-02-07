#pragma once

#include "Vulk.h"
#include "VulkMesh.h"
#include "VulkModel.h"

class VulkActor
{
public:
    std::string name;
    std::shared_ptr<VulkModel> model;
    glm::mat4 xform = glm::mat4(1.0f);
    VulkActor(std::shared_ptr<VulkModel> model, glm::mat4 xform) : model(model), xform(xform) {}
};