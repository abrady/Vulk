#include "Vulk/VulkDescriptorSetLayoutBuilder.h"

#include <iostream>
#include <string>
#include <vector>

#include "Samples/World.h"
#include "Samples/SampleRunner.h"

int main()
{
    SampleRunner<World>("BlinnPhong").run();
    return 0;
}