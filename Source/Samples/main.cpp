#include "Vulk/VulkDescriptorSetLayoutBuilder.h"

#include <iostream>
#include <string>
#include <vector>

#include "Samples/SampleRunner.h"
#include "Samples/World.h"

int main() {
    SampleRunner<World>("BlinnPhong").run();
    return 0;
}