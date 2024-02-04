#include "Vulk/VulkDescriptorSetLayoutBuilder.h"

#include <iostream>
#include <string>
#include <vector>

#include "Samples/PlanarShadowWorld.h"
#include "Samples/SampleRunner.h"

int main()
{
    SampleRunner<PlanarShadowWorld> sample;
    sample.run();
    return 0;
}