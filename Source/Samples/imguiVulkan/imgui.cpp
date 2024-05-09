#include <Vulk/VulkImGui.h>

int main(int, char **) {
    VulkImGui app;
    app.uiRenderer = std::make_shared<ExampleUI>(*app.io);
    app.renderables.push_back(std::make_shared<RenderWorld>());
    app.run();
    return 0;
}
