#include <Vulk/VulkImGui.h>

int main(int, char **) {
    VulkImGui app;
    app.uiRenderer = std::make_shared<ExampleUI>(*app.io);
    app.run();
    return 0;
}
