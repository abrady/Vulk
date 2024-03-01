#include "Vulk/VulkDescriptorSetLayoutBuilder.h"

#include <iostream>
#include <string>
#include <vector>

#include "Samples/SampleRunner.h"
#include "Samples/World.h"

int main() {
    uint32_t apiVersion = 0;
    if (vkEnumerateInstanceVersion(&apiVersion) == VK_SUCCESS) {
        uint32_t major = VK_VERSION_MAJOR(apiVersion);
        uint32_t minor = VK_VERSION_MINOR(apiVersion);
        uint32_t patch = VK_VERSION_PATCH(apiVersion);
        std::cout << "Vulkan version: " << major << "." << minor << "." << patch << std::endl;
    } else {
        std::cout << "Failed to get Vulkan version" << std::endl;
    }

    SampleRunner<World>("ShadowMap").run();
    return 0;
}